#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "file/file.hpp"

namespace astre::editor::model
{
    // Minimal, serialization-friendly view of a chunk.
    struct WorldSnapshot
    {
        void load(file::WorldStreamer & world_streamer)
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
        
        absl::flat_hash_map<file::ChunkID, absl::flat_hash_map<ecs::Entity, ecs::EntityDefinition>> mapping;
    };

    // Deltas are what the editor changes track.
    struct WorldDelta 
    {
        absl::flat_hash_set<file::WorldChunk> created_chunks;
        absl::flat_hash_set<file::ChunkID> removed_chunks;

        absl::flat_hash_map<file::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> created_entities;
        absl::flat_hash_map<file::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> updated_entities;
        absl::flat_hash_map<file::ChunkID, absl::flat_hash_set<ecs::EntityDefinition>> removed_entities;
    };
}