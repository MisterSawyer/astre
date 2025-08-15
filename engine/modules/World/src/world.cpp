#include <cmath>

#include <spdlog/spdlog.h>

#include "world/world.hpp"

namespace astre::world 
{
    static inline ChunkID toChunkID(const math::Vec3 & pos, float size) {
        ChunkID id;
        id.set_x(std::floor(pos.x / size));
        id.set_y(std::floor(pos.y / size));
        id.set_z(std::floor(pos.z / size));        
        return id;
    }

    const absl::flat_hash_set<ChunkID> & WorldStreamer::getAllChunks() const
    {
        return _archive->getAllChunks();
    }

    std::optional<WorldChunk> WorldStreamer::readChunk(const ChunkID& id)
    {
        return _archive->readChunk(id);
    }

    bool WorldStreamer::writeChunk(const WorldChunk & chunk)
    {
        return _archive->writeChunk(chunk);
    }

    bool WorldStreamer::createEntity(const ChunkID & chunk_id, ecs::EntityDefinition entity_def)
    {
        std::size_t entity_id = entity_def.id();
        if(entity_id == ecs::INVALID_ENTITY)
        {
            auto is_unique = [this](const std::size_t & new_entity_id)
            {
                for(const auto & [chunk_id, entities] : _loaded_chunk_entities)
                {
                    if(entities.contains(new_entity_id)) return false;
                }

                return true;
            };

            entity_id = 1;
            while(!is_unique(entity_id))
            {
                ++entity_id;
            }
            
            spdlog::debug("[world] Assigning new entity id: {}", entity_id);
            entity_def.set_id(entity_id);
        }

        _to_reload.emplace(chunk_id);
        return _archive->updateEntity(chunk_id, entity_def);
    }

    bool WorldStreamer::updateEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def)
    {
        if(entity_def.id() == ecs::INVALID_ENTITY)
        {
            spdlog::error("[world] Invalid entity id");
            return false;
        }
        _to_reload.emplace(chunk_id);
        return _archive->updateEntity(chunk_id, entity_def);
    }

    bool WorldStreamer::removeEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def)
    {
        _to_reload.emplace(chunk_id);
        return _archive->removeEntity(chunk_id, entity_def);
    }

    bool WorldStreamer::removeChunk(const ChunkID& id)
    {
        // schedule unloading of chunk
        // it really does not matter when the chunk is unloaded
        // we hold a reference to it in a memory
        asio::co_spawn(_async_context.executor(), _unloadChunk(id), asio::detached);

        // and we can remove this chunk from the archive
        return _archive->removeChunk(id);
    }

    asio::awaitable<void> WorldStreamer::updateLoadPosition(const math::Vec3 & pos)
    {
        co_await _async_context.ensureOnStrand();

        const ChunkID center = toChunkID(pos, _chunk_size);

        const auto & all_available_chunks = getAllChunks();

        absl::flat_hash_set<ChunkID> required;
        for (int dx = -LOAD_RADIUS; dx <= LOAD_RADIUS; ++dx)
        {
            for (int dy = -LOAD_RADIUS; dy <= LOAD_RADIUS; ++dy) 
            {
                for (int dz = -LOAD_RADIUS; dz <= LOAD_RADIUS; ++dz)
                { 
                    ChunkID id;
                    id.set_x(center.x() + dx);
                    id.set_y(center.y() + dy);
                    id.set_z(center.z() + dz);
                    if(all_available_chunks.contains(id) == false) continue;
                    required.insert(std::move(id));
                }
            }
        }

        // Load missing chunks, or reload if needed
        for (const ChunkID& cid : required) {
            if (!_loaded_chunks.contains(cid) || _to_reload.contains(cid))
            {
                co_await _loadChunk(cid);
            }
        }

        // Unload chunks no longer needed
        std::vector<ChunkID> toRemove;
        for (const auto& [cid, _] : _loaded_chunks) {
            if (!required.contains(cid)) {
                toRemove.push_back(cid);
            }
        }
        for (const ChunkID& cid : toRemove) 
        {
            co_await _unloadChunk(cid);
        }

        co_return;
    }

    asio::awaitable<void> WorldStreamer::_loadChunk(const ChunkID& id) 
    {
        co_await _unloadChunk(id);
        co_await _async_context.ensureOnStrand();
        
        const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);

        spdlog::debug("Loading chunk  ({};{};{})", id.x(), id.y(), id.z());

        auto chunk = _archive->readChunk(id);
        if (!chunk.has_value())
        {
            spdlog::error("Failed to load chunk");
            co_return;
        }

        // load chunk entites
        for (const auto& entity_def : (*chunk).entities()) 
        {
            std::optional<ecs::Entity> entity = co_await _registry.spawnEntity(entity_def);
            co_await _async_context.ensureOnStrand();

            if (entity.has_value())
            {
                _loaded_chunk_entities[id].insert(*entity);

                {
                    auto g = asio::experimental::make_parallel_group(
                        asio::co_spawn(_async_context.executor(),  _loader.loadEntity(entity_def, *entity), asio::deferred),
                        asio::co_spawn(_async_context.executor(), _resource_tracker.ensureFor(entity_def), asio::deferred)
                    );
                    auto [ord, e1, e2] =
                        co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);
                    if (e1) std::rethrow_exception(e1);
                    if (e2) std::rethrow_exception(e2);
                }

                co_await _loader.loadEntity(entity_def, *entity);
            }
            else
            {
                spdlog::error("Failed to spawn entity");
            }
        }
        
        co_await _async_context.ensureOnStrand();
        spdlog::debug("Chunk loaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks[id] = std::move(*chunk);
        _to_reload.erase(id);
    }

    asio::awaitable<void> WorldStreamer::_unloadChunk(const ChunkID& id) 
    {
        co_await _async_context.ensureOnStrand();

        if (!_loaded_chunks.contains(id)) co_return;
        if(!_loaded_chunk_entities.contains(id)) co_return;

        spdlog::debug("Unloading chunk  ({};{};{})", id.x(), id.y(), id.z());

        const absl::flat_hash_set<astre::ecs::Entity> chunk = _loaded_chunk_entities.at(id);
        for (ecs::Entity e : chunk)
        {
            co_await _registry.destroyEntity(e);
        }

        co_await _async_context.ensureOnStrand();
        spdlog::debug("Chunk unloaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks.erase(id);
        _loaded_chunk_entities.erase(id);
    }
    
    asio::awaitable<void> WorldStreamer::saveAll()
    {
        co_await _async_context.ensureOnStrand();
        for (const auto& [_, chunk] : _loaded_chunks) {
            _archive->writeChunk(chunk);
        }
    }
    
} // namespace astre::world
