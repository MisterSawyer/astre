#pragma once

#include <filesystem>
#include <memory>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>

#include "async/async.hpp"
#include "process/process.hpp"
#include "math/math.hpp"

#include "file/save_archive.hpp"

#include "proto/ECS/entity_definition.pb.h"
#include "proto/File/world_chunk.pb.h"

namespace astre::file
{
    // load 1 chunk in each direction from streaming position
    static constexpr int LOAD_RADIUS = 1;

    // World state stays outside the uniform asset pipeline (mutable,
    // position-streamed, read/write), but bridges into it structurally: it
    // exposes read(ChunkID) -> const WorldChunk*, satisfying the Asset
    // module's DefinitionSource concept without inheriting from or
    // depending on it (see Loader, which checks this statically since it's
    // the module that actually depends on both File and Asset).
    class WorldStreamer
    {
        public:
            template<class Mode>
            WorldStreamer(
                process::IProcess::execution_context_type & execution_context,
                std::filesystem::path path,
                Mode mode,
                float chunk_size,
                unsigned int max_loaded_chunks
            )
            :   _async_context(execution_context),
                _archive(std::make_unique<SaveArchive<Mode>>(std::move(path))),
                _chunk_size(chunk_size),
                _max_loaded_chunks(max_loaded_chunks)
            {}

            const absl::flat_hash_set<proto::file::ChunkID> & getAllChunks() const;

            asio::awaitable<void> updateLoadPosition(const math::Vec3 & pos);
            
            const proto::file::WorldChunk * read(proto::file::ChunkID id) const;
            bool write(const proto::file::WorldChunk & chunk);
            bool remove(proto::file::ChunkID id);

        private:
            asio::awaitable<void> _loadChunk(const proto::file::ChunkID& id);
            asio::awaitable<void> _unloadChunk(const proto::file::ChunkID& id);

            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            std::unique_ptr<ISaveArchive> _archive;

            float _chunk_size;
            unsigned int _max_loaded_chunks;

            absl::flat_hash_map<proto::file::ChunkID, std::unique_ptr<proto::file::WorldChunk>> _loaded_chunks;
            absl::flat_hash_set<proto::file::ChunkID> _to_reload;
    };
}