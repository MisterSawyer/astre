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
            async::AsyncContext<process::IProcess::execution_context_type> & async_context,
            std::filesystem::path path,
            ecs::Registry& registry,
            asset::EntityLoader& loader,
            asset::EntitySerializer& serializer,
            float chunk_size,
            unsigned int max_loaded_chunks);

        asio::awaitable<void> addEntitiesToChunk(ChunkID chunk_id, std::vector<ecs::Entity> entities);

        asio::awaitable<void> updateLoadPosition(const math::Vec3 & pos);
        asio::awaitable<void> saveAll(asset::use_binary_t);
        asio::awaitable<void> saveAll(asset::use_json_t);

    private:
        void loadChunk(const ChunkID& id);
        void unloadChunk(const ChunkID& id);

        async::AsyncContext<process::IProcess::execution_context_type> & _async_context;

        ecs::Registry& _registry;
        asset::EntityLoader & _loader;
        asset::EntitySerializer & _serializer;

        SaveArchive _archive;

        float _chunk_size;
        unsigned int _max_loaded_chunks;

        absl::flat_hash_map<ChunkID, WorldChunk> _loaded_chunks;
        absl::flat_hash_map<ChunkID, absl::flat_hash_set<ecs::Entity>> _chunk_entities;
    };
}