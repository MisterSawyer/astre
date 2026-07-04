#include "loader/chunk_loader.hpp"

#include <algorithm>
#include <exception>
#include <utility>

#include <spdlog/spdlog.h>

#include "math/math.hpp"

namespace astre::loader
{
    asio::awaitable<bool> ChunkLoader::sync(absl::flat_hash_set<proto::file::ChunkID> keys, asset::WorldStreamer & world_streamer)
    {
        for(const auto & key : keys)
        {
            if(_loaded_chunks.contains(key)) continue;

            const auto * chunk = world_streamer.read(key);
            if(!chunk) continue;
            if(!co_await load(*chunk)) co_return false;
        }

        // Re-home entities that crossed a chunk border. The stale pass below
        // only fires when a chunk leaves the radius, so at LOAD_RADIUS >= 1 a
        // border cross between two resident chunks needs this. Entities that
        // crossed into an unloaded chunk get re-homed here too, then unloaded by
        // the stale pass (their new chunk is not required, so it goes stale).
        if(!co_await _migrateMovedEntities(world_streamer, _entity_loader.registry())) co_return false;

        std::vector<proto::file::ChunkID> stale;
        for(const auto & key : _loaded_chunks)
            if(!keys.contains(key))
                stale.push_back(key);

        if(!stale.empty())
        {
            if(!co_await _syncStaleChunkEntities(world_streamer, _entity_loader.registry(), stale)) co_return false;
            if(!co_await unload(stale)) co_return false;
            if(!co_await world_streamer.unloadCachedChunks(stale)) co_return false;
        }

        co_return true;
    }

    asio::awaitable<bool> ChunkLoader::load(const proto::file::WorldChunk & chunk)
    {
        if(_chunk_entities.contains(chunk.id()))
        {
            spdlog::error("[chunk-loader] Chunk ({}, {}, {}) already loaded",
                chunk.id().x(), chunk.id().y(), chunk.id().z());
            co_return false;
        }

        std::vector<ecs::Entity> loaded_entities;
        loaded_entities.reserve(chunk.entities().size());

        for(const auto & entity_def : chunk.entities())
        {
            if(!co_await _entity_loader.load(entity_def))
            {
                co_await _entity_loader.unload(loaded_entities);
                for(const auto entity : loaded_entities)
                    _entity_chunks.erase(entity);
                co_return false;
            }
            loaded_entities.push_back(entity_def.id());
            _entity_chunks[entity_def.id()] = chunk.id();
        }

        _chunk_entities.emplace(chunk.id(), std::move(loaded_entities));
        _loaded_chunks.insert(chunk.id());
        co_return true;
    }

    asio::awaitable<bool> ChunkLoader::unload(const proto::file::ChunkID & chunk)
    {
        co_return co_await unload(std::vector<proto::file::ChunkID>{chunk});
    }

    asio::awaitable<bool> ChunkLoader::unload(const std::vector<proto::file::ChunkID> & chunks)
    {
        std::vector<ecs::Entity> entities;
        for(const auto & chunk : chunks)
        {
            auto it = _chunk_entities.find(chunk);
            if(it == _chunk_entities.end())
            {
                spdlog::warn("[chunk-loader] Chunk ({}, {}, {}) was not loaded",
                    chunk.x(), chunk.y(), chunk.z());
                _loaded_chunks.erase(chunk);
                continue;
            }

            entities.insert(entities.end(), it->second.begin(), it->second.end());
            for(const auto entity : it->second)
                _entity_chunks.erase(entity);
            _chunk_entities.erase(it);
            _loaded_chunks.erase(chunk);
        }

        if(!entities.empty())
            co_return co_await _entity_loader.unload(entities);

        co_return true;
    }

    std::optional<proto::file::ChunkID> ChunkLoader::_cachedTransformChunk(
        asset::WorldStreamer & world_streamer,
        const proto::file::ChunkID & chunk_id,
        ecs::Entity entity) const
    {
        const auto * chunk = world_streamer.read(chunk_id);
        if(!chunk) return std::nullopt;

        for(const auto & entity_def : chunk->entities())
        {
            if(entity_def.id() != entity || !entity_def.has_transform()) continue;
            return world_streamer.chunkIdForPosition(math::deserialize(entity_def.transform().position()));
        }

        return std::nullopt;
    }

    asio::awaitable<bool> ChunkLoader::_syncStaleChunkEntities(
        asset::WorldStreamer & world_streamer,
        const ecs::Registry & registry,
        const std::vector<proto::file::ChunkID> & stale)
    {
        EntitySerializer serializer;
        std::vector<std::pair<ecs::Entity, proto::file::ChunkID>> owned;
        for(const auto & chunk : stale)
        {
            auto it = _chunk_entities.find(chunk);
            if(it == _chunk_entities.end()) continue;

            for(const auto entity : it->second)
                owned.emplace_back(entity, chunk);
        }

        for(const auto & [entity, old_chunk] : owned)
        {
            proto::ecs::EntityDefinition entity_def;
            try
            {
                entity_def = co_await serializer.serializeEntity(entity, registry);
            }
            catch(const std::exception & ex)
            {
                spdlog::error("[chunk-loader] Failed to serialize entity {} while moving chunks: {}", entity, ex.what());
                co_return false;
            }

            if(!entity_def.has_transform()) continue;

            const auto new_chunk = world_streamer.chunkIdForPosition(math::deserialize(entity_def.transform().position()));
            if(new_chunk == old_chunk)
            {
                if(!co_await world_streamer.upsertCachedEntity(old_chunk, std::move(entity_def))) co_return false;
                continue;
            }

            const auto cached_chunk = _cachedTransformChunk(world_streamer, old_chunk, entity);
            if(cached_chunk.has_value() && *cached_chunk == new_chunk) continue;

            if(!co_await _rehomeEntity(world_streamer, entity, std::move(entity_def), old_chunk, new_chunk)) co_return false;
        }

        co_return true;
    }

    std::optional<proto::file::ChunkID> ChunkLoader::_liveChunk(
        asset::WorldStreamer & world_streamer,
        const ecs::Registry & registry,
        ecs::Entity entity) const
    {
        std::optional<proto::file::ChunkID> result;
        registry.runOnSingleWithComponents<proto::ecs::TransformComponent>(entity,
            [&](const ecs::Entity, const proto::ecs::TransformComponent & transform)
            {
                result = world_streamer.chunkIdForPosition(math::deserialize(transform.position()));
            });
        return result;
    }

    asio::awaitable<bool> ChunkLoader::_rehomeEntity(
        asset::WorldStreamer & world_streamer,
        ecs::Entity entity,
        proto::ecs::EntityDefinition entity_def,
        const proto::file::ChunkID & old_chunk,
        const proto::file::ChunkID & new_chunk)
    {
        if(!co_await world_streamer.removeCachedEntity(old_chunk, entity)) co_return false;
        if(!co_await world_streamer.upsertCachedEntity(new_chunk, std::move(entity_def))) co_return false;

        auto old_it = _chunk_entities.find(old_chunk);
        if(old_it != _chunk_entities.end())
        {
            auto & entities = old_it->second;
            entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
        }

        _chunk_entities[new_chunk].push_back(entity);
        _loaded_chunks.insert(new_chunk);
        _entity_chunks[entity] = new_chunk;

        spdlog::debug("[chunk-loader] Entity {} moved from chunk ({}, {}, {}) to ({}, {}, {})",
            entity,
            old_chunk.x(), old_chunk.y(), old_chunk.z(),
            new_chunk.x(), new_chunk.y(), new_chunk.z());
        co_return true;
    }

    asio::awaitable<bool> ChunkLoader::_migrateMovedEntities(
        asset::WorldStreamer & world_streamer,
        const ecs::Registry & registry)
    {
        // Cheap transform-only scan first; re-homing mutates _entity_chunks, so
        // snapshot the movers before touching anything.
        std::vector<std::pair<ecs::Entity, proto::file::ChunkID>> moved;
        for(const auto & [entity, old_chunk] : _entity_chunks)
        {
            const auto live = _liveChunk(world_streamer, registry, entity);
            if(live && *live != old_chunk)
                moved.emplace_back(entity, old_chunk);
        }

        EntitySerializer serializer;
        for(const auto & [entity, old_chunk] : moved)
        {
            proto::ecs::EntityDefinition entity_def;
            try
            {
                entity_def = co_await serializer.serializeEntity(entity, registry);
            }
            catch(const std::exception & ex)
            {
                spdlog::error("[chunk-loader] Failed to serialize entity {} while migrating chunks: {}", entity, ex.what());
                co_return false;
            }

            if(!entity_def.has_transform()) continue;

            const auto new_chunk = world_streamer.chunkIdForPosition(math::deserialize(entity_def.transform().position()));
            if(new_chunk == old_chunk) continue;

            if(!co_await _rehomeEntity(world_streamer, entity, std::move(entity_def), old_chunk, new_chunk)) co_return false;
        }

        co_return true;
    }
}
