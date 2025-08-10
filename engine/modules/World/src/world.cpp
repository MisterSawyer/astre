#include <cmath>

#include <spdlog/spdlog.h>

#include "world/world.hpp"

namespace astre::world {

    static constexpr int LOAD_RADIUS = 0;

    WorldStreamer::WorldStreamer(
        process::IProcess::execution_context_type & execution_context,
        std::filesystem::path path,
        ecs::Registry& registry,
        float chunk_size,
        unsigned int max_loaded_chunks
    )
        :   _async_context(execution_context),
            _archive(std::move(path), asset::use_json),
            _registry(registry),
            _loader(registry),
            _serializer(registry),
            _chunk_size(chunk_size),
            _max_loaded_chunks(max_loaded_chunks)
    {}

    static inline ChunkID toChunkID(const math::Vec3 & pos, float size) {
        ChunkID id;
        id.set_x(std::floor(pos.x / size));
        id.set_y(std::floor(pos.y / size));
        id.set_z(std::floor(pos.z / size));        
        return id;
    }

    const absl::flat_hash_set<ChunkID> & WorldStreamer::getAllChunks() const
    {
        return _archive.getAllChunks();
    }

    std::optional<WorldChunk> WorldStreamer::readChunk(const ChunkID& id, asset::use_binary_t)
    {
        return _archive.readChunk(id, asset::use_binary);
    }

    std::optional<WorldChunk> WorldStreamer::readChunk(const ChunkID& id, asset::use_json_t)
    {
        return _archive.readChunk(id, asset::use_json);
    }

    bool WorldStreamer::updateEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def, asset::use_binary_t)
    {
        _to_reload.emplace(chunk_id);
        return _archive.updateEntity(chunk_id, entity_def, asset::use_binary);
    }

    bool WorldStreamer::updateEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def, asset::use_json_t)
    {
        _to_reload.emplace(chunk_id);
        return _archive.updateEntity(chunk_id, entity_def, asset::use_json);
    }

    asio::awaitable<void> WorldStreamer::updateLoadPosition(const math::Vec3 & pos)
    {
        co_await _async_context.ensureOnStrand();

        const ChunkID center = toChunkID(pos, _chunk_size);

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
                    required.insert(std::move(id));
                }
            }
        }

        // Load missing chunks, or reload if needed
        for (const ChunkID& cid : required) {
            if (!_loaded_chunks.contains(cid) || _to_reload.contains(cid))
            {
                co_await loadChunk(cid);
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
            co_await unloadChunk(cid);
        }

        co_return;
    }

    asio::awaitable<void> WorldStreamer::loadChunk(const ChunkID& id) 
    {
        co_await unloadChunk(id);
        co_await _async_context.ensureOnStrand();

        spdlog::debug("Loading chunk  ({};{};{})", id.x(), id.y(), id.z());

        auto chunk = _archive.readChunk(id, asset::use_json);
        if (!chunk.has_value())
        {
            spdlog::error("Failed to load chunk");
            co_return;
        }

        // load chunk entites
        for (const auto& entity_def : (*chunk).entities()) 
        {
            std::optional<ecs::Entity> entity = co_await _registry.createEntity(entity_def.name());
            co_await _async_context.ensureOnStrand();

            if (entity.has_value()) {
                _loaded_chunk_entities[id].insert(*entity);
                co_await _loader.loadEntity(entity_def, *entity);
            }
        }
        
        co_await _async_context.ensureOnStrand();
        spdlog::debug("Chunk loaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks[id] = std::move(*chunk);
        _to_reload.erase(id);
    }

    asio::awaitable<void> WorldStreamer::unloadChunk(const ChunkID& id) 
    {
        co_await _async_context.ensureOnStrand();
        spdlog::debug("Unloading chunk  ({};{};{})", id.x(), id.y(), id.z());

        if (!_loaded_chunks.contains(id)) co_return;
        if(!_loaded_chunk_entities.contains(id)) co_return;

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
    
    asio::awaitable<void> WorldStreamer::saveAll(asset::use_binary_t)
    {
        co_await _async_context.ensureOnStrand();
        for (const auto& [_, chunk] : _loaded_chunks) {
            _archive.writeChunk(chunk, asset::use_binary);
        }
    }

    asio::awaitable<void> WorldStreamer::saveAll(asset::use_json_t)
    {
        co_await _async_context.ensureOnStrand();
        for (const auto& [_, chunk] : _loaded_chunks) {
            _archive.writeChunk(chunk, asset::use_json);
        }
    }
    
} // namespace astre::world
