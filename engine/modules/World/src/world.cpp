#include <cmath>

#include "world/world.hpp"

namespace astre::world {

    static constexpr std::int32_t LOAD_RADIUS = 2;

    WorldStreamer::WorldStreamer(ecs::Registry& registry,
                                 asset::EntityLoader& loader,
                                 asset::EntitySerializer& serializer,
                                 SaveArchive& archive,
                                 float chunk_size,
                                 unsigned int max_loaded_chunks)
        : _registry(registry),
          _loader(loader),
          _serializer(serializer),
          _archive(archive),
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

    void WorldStreamer::updatePlayerPosition(const math::Vec3 & pos) {
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
    }

    void WorldStreamer::loadChunk(const ChunkID& id) {
        auto defs = _archive.readChunk(id, asset::use_binary);
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

    void WorldStreamer::saveAll() {
        for (const auto& [chunk, entities] : _loaded) {
            std::vector<ecs::EntityDefinition> defs;
            for (ecs::Entity e : entities) {
                defs.emplace_back(_serializer.serializeEntity(e, _registry));
            }
            _archive.writeChunk(chunk, defs, asset::use_binary);
        }
        _archive.saveIndex();
    }

} // namespace astre::world
