#include <cmath>

#include <spdlog/spdlog.h>

#include "world/world.hpp"

namespace astre::world {

    static constexpr std::int32_t LOAD_RADIUS = 2;

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
            _archive(std::move(path)), 
            _registry(registry),
            _loader(loader),
            _serializer(serializer),
            _chunk_size(chunk_size),
            _max_loaded_chunks(max_loaded_chunks)
    {}

    static inline ChunkID toChunkID(const math::Vec3& pos, float size) {
        return ChunkID {
            static_cast<int>(std::floor(pos.x / size)),
            static_cast<int>(std::floor(pos.y / size)),
            static_cast<int>(std::floor(pos.z / size))
        };
    }

    asio::awaitable<void> WorldStreamer::addEntityToChunk(ecs::Entity entity, ChunkID chunk_id)
    {
        co_await _async_context.ensureOnStrand();
        _loaded[chunk_id].insert(entity);
    }

    asio::awaitable<void> WorldStreamer::updateLoadPosition(const math::Vec3 & pos)
    {
        co_await _async_context.ensureOnStrand();

        const ChunkID center = toChunkID(pos, _chunk_size);

        absl::flat_hash_set<ChunkID> required;
        for (int dx = -LOAD_RADIUS; dx <= LOAD_RADIUS; ++dx) {
            for (int dy = -LOAD_RADIUS; dy <= LOAD_RADIUS; ++dy) {
                required.insert({ center.x + dx, center.y + dy });
            }
        }

        // Load missing chunks
        for (const ChunkID& cid : required) {
            if (!_loaded.contains(cid)) {
                loadChunk(cid);
            }
        }

        // Unload chunks no longer needed
        std::vector<ChunkID> toRemove;
        for (const auto& [cid, _] : _loaded) {
            if (!required.contains(cid)) {
                toRemove.push_back(cid);
            }
        }
        for (const ChunkID& cid : toRemove) {
            unloadChunk(cid);
            _loaded.erase(cid);
        }

        co_return;
    }

    void WorldStreamer::loadChunk(const ChunkID& id) 
    {
        auto defs = _archive.readChunk(id, asset::use_json);
        if (!defs.has_value()) return;

        absl::flat_hash_set<ecs::Entity> chunk_entities;
        for (const auto& def : defs.value()) {
            auto entity = _registry.createEntity(def.name());
            if (entity.has_value()) {
                _loader.loadEntity(def, *entity, _registry);
                chunk_entities.insert(*entity);
            }
        }

        _loaded[id] = std::move(chunk_entities);
    }

    void WorldStreamer::unloadChunk(const ChunkID& id) {
        if (!_loaded.contains(id)) return;
        for (ecs::Entity e : _loaded[id]) {
            _registry.destroyEntity(e);
        }
    }
    
    std::vector<ecs::EntityDefinition> WorldStreamer::createEntityDeps(const absl::flat_hash_set<ecs::Entity> & entities) const
    {
        std::vector<ecs::EntityDefinition> defs;
        for (ecs::Entity e : entities) {
            defs.emplace_back(_serializer.serializeEntity(e, _registry));
        }
        return defs;
    }

    asio::awaitable<void> WorldStreamer::saveAll(asset::use_binary_t)
    {
        co_await _async_context.ensureOnStrand();
        for (const auto& [chunk, entities] : _loaded) {
            _archive.writeChunk(chunk, createEntityDeps(entities), asset::use_binary);
        }
        _archive.saveIndex();
    }

    asio::awaitable<void> WorldStreamer::saveAll(asset::use_json_t)
    {
        co_await _async_context.ensureOnStrand();
        for (const auto& [chunk, entities] : _loaded) {
            _archive.writeChunk(chunk, createEntityDeps(entities), asset::use_json);
        }
    }
    
} // namespace astre::world
