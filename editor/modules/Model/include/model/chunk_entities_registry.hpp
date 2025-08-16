#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "world/world.hpp"

namespace astre::editor::model
{
    struct ChunkEntityRegistry
    {
        void update(world::WorldStreamer & world_streamer)
        {
            mapping.clear();
        
            for(const auto & chunk_id : world_streamer.getAllChunks())
            {
                mapping.emplace(chunk_id, absl::flat_hash_map<ecs::Entity, ecs::EntityDefinition>());

            // then for every node we obtain chunk definition
            auto chunk_res = world_streamer.readChunk(chunk_id);
                        
                if (chunk_res.has_value())
                {
                    const auto & chunk = *chunk_res;

                    // then for every entity in chunk
                    for(const auto & entity_def : chunk.entities())
                    {
                        mapping.at(chunk_id).emplace(entity_def.id(), entity_def);
                    }
                }
            }
        }
        
        absl::flat_hash_map<world::ChunkID, absl::flat_hash_map<ecs::Entity, ecs::EntityDefinition>> mapping;
    };

}