#pragma once

#include <filesystem>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_set.h>

#include "async/async.hpp"
#include "process/process.hpp"
#include "ecs/ecs.hpp"
#include "asset/asset.hpp"

#include "world/save_archive.hpp"

#include "generated/World/proto/world_chunk.pb.h"

namespace astre::world
{
    class WorldStreamer {
    public:
        WorldStreamer(
            process::IProcess::execution_context_type & execution_context,
            std::filesystem::path path,
            ecs::Registry& registry,
            float chunk_size,
            unsigned int max_loaded_chunks);

        const absl::flat_hash_set<ChunkID> & getAllChunks() const;

        asio::awaitable<void> updateLoadPosition(const math::Vec3 & pos);
        asio::awaitable<void> saveAll(asset::use_binary_t);
        asio::awaitable<void> saveAll(asset::use_json_t);

        std::optional<WorldChunk> readChunk(const ChunkID& id, asset::use_binary_t);
        std::optional<WorldChunk> readChunk(const ChunkID& id, asset::use_json_t);

        bool updateEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def, asset::use_binary_t);
        bool updateEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def, asset::use_json_t);

    private:
        asio::awaitable<void> loadChunk(const ChunkID& id);
        asio::awaitable<void> unloadChunk(const ChunkID& id);

        async::AsyncContext<process::IProcess::execution_context_type> _async_context;

        ecs::Registry& _registry;
        asset::EntityLoader  _loader;
        asset::EntitySerializer _serializer;

        SaveArchive _archive;

        float _chunk_size;
        unsigned int _max_loaded_chunks;


        absl::flat_hash_map<ChunkID, WorldChunk> _loaded_chunks;
        absl::flat_hash_set<ChunkID> _to_reload;
        absl::flat_hash_map<ChunkID, absl::flat_hash_set<ecs::Entity>> _loaded_chunk_entities;
    };
}