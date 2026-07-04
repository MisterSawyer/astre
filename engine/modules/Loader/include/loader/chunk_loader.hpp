#pragma once

#include <optional>
#include <vector>

#include <asio.hpp>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "asset/world_streamer.hpp"
#include "file/world_file.hpp"
#include "loader/entity_loader.hpp"
#include "loader/entity_serializer.hpp"

namespace astre::loader
{
    // Stage 3 sink for world streaming. A WorldChunk is a container of entity
    // definitions, so loading one chunk fans out into an EntityLoader::load per
    // entity.
    class ChunkLoader
    {
    public:
        explicit ChunkLoader(EntityLoader & entity_loader) : _entity_loader(entity_loader) {}

        asio::awaitable<bool> sync(absl::flat_hash_set<proto::file::ChunkID> keys, asset::WorldStreamer & world_streamer);

        template<class Keys, class Source>
        asio::awaitable<bool> sync(Keys keys, const Source & source)
        {
            std::vector<proto::file::ChunkID> stale;
            for(const auto & key : _loaded_chunks)
                if(!keys.contains(key))
                    stale.push_back(key);

            if(!stale.empty())
                if(!co_await unload(stale)) co_return false;

            for(const auto & key : keys)
            {
                if(_loaded_chunks.contains(key)) continue;

                const auto * chunk = source.read(key);
                if(!chunk) continue;
                if(!co_await load(*chunk)) co_return false;
            }

            co_return true;
        }

        asio::awaitable<bool> load(const proto::file::WorldChunk & chunk);
        asio::awaitable<bool> unload(const std::vector<proto::file::ChunkID> & chunks);
        asio::awaitable<bool> unload(const proto::file::ChunkID & chunk);

    private:
        std::optional<proto::file::ChunkID> _cachedTransformChunk(
            asset::WorldStreamer & world_streamer,
            const proto::file::ChunkID & chunk_id,
            ecs::Entity entity) const;

        asio::awaitable<bool> _syncStaleChunkEntities(
            asset::WorldStreamer & world_streamer,
            const ecs::Registry & registry,
            const std::vector<proto::file::ChunkID> & stale);

        // Current chunk of an entity's live transform, or nullopt if it has none.
        std::optional<proto::file::ChunkID> _liveChunk(
            asset::WorldStreamer & world_streamer,
            const ecs::Registry & registry,
            ecs::Entity entity) const;

        // Move entity's definition and bookkeeping from old_chunk to new_chunk.
        asio::awaitable<bool> _rehomeEntity(
            asset::WorldStreamer & world_streamer,
            ecs::Entity entity,
            proto::ecs::EntityDefinition entity_def,
            const proto::file::ChunkID & old_chunk,
            const proto::file::ChunkID & new_chunk);

        // Re-home entities whose live position crossed into a different chunk. If
        // the new chunk is outside the radius the stale pass unloads it afterwards.
        asio::awaitable<bool> _migrateMovedEntities(
            asset::WorldStreamer & world_streamer,
            const ecs::Registry & registry);

        EntityLoader & _entity_loader;
        absl::flat_hash_map<proto::file::ChunkID, std::vector<ecs::Entity>> _chunk_entities;
        absl::flat_hash_map<ecs::Entity, proto::file::ChunkID> _entity_chunks;
        absl::flat_hash_set<proto::file::ChunkID> _loaded_chunks;
    };
}
