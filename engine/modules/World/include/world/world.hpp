#pragma once

#include <absl/container/flat_hash_set.h>

#include "ecs/ecs.hpp"
#include "asset/asset.hpp"

#include "world/chunk.hpp"
#include "world/save_archive.hpp"


namespace astre::world
{
    class WorldStreamer {
    public:
        WorldStreamer(ecs::Registry& registry,
                      asset::EntityLoader& loader,
                      asset::EntitySerializer& serializer,
                      SaveArchive& archive,
                      float chunk_size,
                      unsigned int max_loaded_chunks);

        void updatePlayerPosition(const math::Vec3 & pos);
        void saveAll();

    private:
        void loadChunk(const ChunkID& id);
        void unloadChunk(const ChunkID& id);

        ecs::Registry& _registry;
        asset::EntityLoader & _loader;
        asset::EntitySerializer & _serializer;
        SaveArchive& _archive;

        float _chunk_size;
        unsigned int _max_loaded_chunks;

        absl::flat_hash_map<ChunkID, absl::flat_hash_set<ecs::Entity>> _loaded;
    };
}