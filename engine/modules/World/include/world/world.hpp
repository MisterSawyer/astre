#pragma once

#include <filesystem>
#include <memory>

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
    static constexpr int LOAD_RADIUS = 0;

    class WorldStreamer
    {
        public:

            template<class Mode>
            WorldStreamer(
                process::IProcess::execution_context_type & execution_context,
                Mode mode,
                std::filesystem::path path,
                ecs::Registry& registry,
                float chunk_size,
                unsigned int max_loaded_chunks
            )
            :   _async_context(execution_context),
                _archive(std::make_unique<SaveArchive<Mode>>(std::move(path))),
                _registry(registry),
                _loader(registry),
                _serializer(registry),
                _chunk_size(chunk_size),
                _max_loaded_chunks(max_loaded_chunks)
            {}

            const absl::flat_hash_set<ChunkID> & getAllChunks() const;

            asio::awaitable<void> updateLoadPosition(const math::Vec3 & pos);

            asio::awaitable<void> saveAll();

            bool writeChunk(const WorldChunk & chunk);

            std::optional<WorldChunk> readChunk(const ChunkID& id);

            bool removeChunk(const ChunkID& id);

            // will add entity if does not exist
            bool updateEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def);

            bool removeEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def);

        private:
            asio::awaitable<void> _loadChunk(const ChunkID& id);
            asio::awaitable<void> _unloadChunk(const ChunkID& id);

            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            ecs::Registry& _registry;
            asset::EntityLoader  _loader;
            asset::EntitySerializer _serializer;

            std::unique_ptr<ISaveArchive> _archive;

            float _chunk_size;
            unsigned int _max_loaded_chunks;

            absl::flat_hash_map<ChunkID, WorldChunk> _loaded_chunks;
            absl::flat_hash_set<ChunkID> _to_reload;
            absl::flat_hash_map<ChunkID, absl::flat_hash_set<ecs::Entity>> _loaded_chunk_entities;
    };
}