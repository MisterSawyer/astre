#include <cmath>

#include <spdlog/spdlog.h>

#include "world/world.hpp"

namespace astre::world {

    static constexpr int LOAD_RADIUS = 0;

    WorldStreamer::WorldStreamer(
        async::AsyncContext<process::IProcess::execution_context_type> & async_context,
        std::filesystem::path path,
        ecs::Registry& registry,
        asset::EntityLoader& loader,
        asset::EntitySerializer& serializer,
        float chunk_size,
        unsigned int max_loaded_chunks
    )
        :   _async_context(async_context),
            _archive(std::move(path), asset::use_json),
            _registry(registry),
            _loader(loader),
            _serializer(serializer),
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

    asio::awaitable<void> WorldStreamer::addEntitiesToChunk(ChunkID chunk_id, std::vector<ecs::Entity> entities)
    {
        co_await _async_context.ensureOnStrand();
        for(const auto & e : entities)
        {
            _chunk_entities[chunk_id].insert(e);
            _loaded_chunks[chunk_id].add_entities()->CopyFrom(_serializer.serializeEntity(e, _registry));
        }
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

        // Load missing chunks
        for (const ChunkID& cid : required) {
            if (!_loaded_chunks.contains(cid)) {
                loadChunk(cid);
            }
        }

        // Unload chunks no longer needed
        std::vector<ChunkID> toRemove;
        for (const auto& [cid, _] : _loaded_chunks) {
            if (!required.contains(cid)) {
                toRemove.push_back(cid);
            }
        }
        for (const ChunkID& cid : toRemove) {
            unloadChunk(cid);
        }

        co_return;
    }

    void WorldStreamer::loadChunk(const ChunkID& id) 
    {
        auto chunk = _archive.readChunk(id, asset::use_json);
        if (!chunk.has_value())
        {
            spdlog::error("Failed to load chunk");
            return;
        }

        // load chunk entites
        for (const auto& entity_def : (*chunk).entities()) {
            auto entity = _registry.createEntity(entity_def.name());
            if (entity.has_value()) {
                _loader.loadEntity(entity_def, *entity, _registry);
                _chunk_entities[id].insert(*entity);
            }
        }
        spdlog::debug("Chunk loaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks[id] = std::move(*chunk);
    }

    void WorldStreamer::unloadChunk(const ChunkID& id) {
        if (!_loaded_chunks.contains(id)) return;
        for (ecs::Entity e : _chunk_entities[id]) {
            _registry.destroyEntity(e);
        }
        spdlog::debug("Chunk unloaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks.erase(id);
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
