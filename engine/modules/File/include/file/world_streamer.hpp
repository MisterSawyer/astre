#pragma once

#include <filesystem>
#include <memory>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_set.h>

#include "async/async.hpp"
#include "process/process.hpp"
#include "math/math.hpp"

#include "file/save_archive.hpp"

#include "generated/ECS/proto/entity_definition.pb.h"
#include "generated/File/proto/world_chunk.pb.h"

namespace astre::file
{
    // load 1 chunk in each direction from streaming position
    static constexpr int LOAD_RADIUS = 1;

    class WorldStreamer
    {
        public:
            template<class Mode>
            WorldStreamer(
                process::IProcess::execution_context_type & execution_context,
                Mode mode,
                std::filesystem::path path,
                float chunk_size,
                unsigned int max_loaded_chunks
            )
            :   _async_context(execution_context),
                _archive(std::make_unique<SaveArchive<Mode>>(std::move(path))),
                _chunk_size(chunk_size),
                _max_loaded_chunks(max_loaded_chunks)
            {}

            const absl::flat_hash_set<ChunkID> & getAllChunks() const;

            asio::awaitable<void> updateLoadPosition(const math::Vec3 & pos);

            asio::awaitable<void> saveAll();

            bool writeChunk(const WorldChunk & chunk);

            std::optional<WorldChunk> readChunk(const ChunkID& id);

            bool removeChunk(const ChunkID& id);


            bool writeEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def);

            bool removeEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def);

        private:
            asio::awaitable<void> _loadChunk(const ChunkID& id);
            asio::awaitable<void> _unloadChunk(const ChunkID& id);

            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            std::unique_ptr<ISaveArchive> _archive;

            float _chunk_size;
            unsigned int _max_loaded_chunks;

            absl::flat_hash_map<ChunkID, WorldChunk> _loaded_chunks;
            absl::flat_hash_set<ChunkID> _to_reload;
    };
}