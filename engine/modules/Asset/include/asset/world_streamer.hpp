#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>

#include "async/async.hpp"
#include "process/process.hpp"
#include "math/math.hpp"

#include "ecs/entity.hpp"
#include "file/world_file.hpp"

#include "asset/asset_cache.hpp"
#include "asset/asset_streamer.hpp"

#include "proto/ECS/entity_definition.pb.h"
#include "proto/File/world_chunk.pb.h"

namespace astre::asset
{
    // load X chunk in each direction from streaming position
    // when 0 load only the chunk at streaming position
    static constexpr int LOAD_RADIUS = 1;

    // A writable, position-streamed asset. Like ShaderStreamer/ScriptStreamer it
    // opens no file at construction, holding only an AssetCache — _archive is null
    // until stream(file, mode) opens the WorldFile. After that, chunks are imported
    // from the file::IWorldFile (Stage 1) into the cache (Stage 2) via the same
    // asset::streamAssets the other streamers use, as the load position moves, and
    // evicted when they leave the radius. read(ChunkID) makes it a DefinitionSource
    // like any other cache; write()/remove() are the read+write half no immutable
    // asset has. It has no strand of its own — all its state runs on the cache's
    // strand, so cache access stays single-strand.
    class WorldStreamer
    {
        public:
            // Per-chunk streaming state for debug visualization. Precedence when a
            // chunk sits in several sets: Dirty > ToReload > Loaded > Required > Unloaded.
            enum class ChunkDebugState { Unloaded, Required, Loaded, ToReload, Dirty };

            WorldStreamer(
                process::IProcess & process,
                float chunk_size
            )
            :   _archive(nullptr),
                _cache(process),
                _chunk_size(chunk_size)
            {}

            // Opens the WorldFile at `file_path` in the given format
            // (file::use_json / use_binary), the world analogue of
            // ShaderStreamer::stream(files). Returns false if the archive could not
            // be opened/parsed (leaving _archive null, so updateLoadPosition/write/
            // remove stay no-ops). The archive scan is blocking, so it runs on the
            // pool.
            // one archive for now; a future multi-file world would keep a
            // map keyed by name, opened by repeated stream() calls.
            template<class Mode>
            asio::awaitable<bool> stream(std::filesystem::path file_path, Mode mode)
            {
                co_await asio::post(_cache.executor(), asio::use_awaitable);
                _archive = file::openWorldArchive(std::move(file_path), mode);
                co_return _archive != nullptr;
            }

            // Runtime sync keys. Stale cached chunks may stay resident internally
            // until ChunkLoader has migrated/unloaded their entities.
            absl::flat_hash_set<proto::file::ChunkID> keys() const { return _required_chunks; }

            const absl::flat_hash_set<proto::file::ChunkID> & getAllChunks() const;

            float chunkSize() const { return _chunk_size; }

            // Snapshot every known chunk with its debug state, for the chunk-border
            // overlay. Runs on the cache strand so cache access stays single-strand.
            asio::awaitable<absl::flat_hash_map<proto::file::ChunkID, ChunkDebugState>> snapshotChunkStates() const;

            asio::awaitable<void> updateLoadPosition(const math::Vec3 & pos);
            proto::file::ChunkID chunkIdForPosition(const math::Vec3 & pos) const;
            std::optional<math::Vec3> findEntityPosition(ecs::Entity entity) const;

            const proto::file::WorldChunk * read(proto::file::ChunkID id) const;
            asio::awaitable<bool> write(const proto::file::WorldChunk & chunk);
            asio::awaitable<bool> remove(proto::file::ChunkID id);
            asio::awaitable<bool> upsertCachedEntity(proto::file::ChunkID id, proto::ecs::EntityDefinition entity_def);
            asio::awaitable<bool> removeCachedEntity(proto::file::ChunkID id, ecs::Entity entity);
            asio::awaitable<bool> unloadCachedChunks(const std::vector<proto::file::ChunkID> & ids);

            // Flush every dirty cached chunk to the archive. Call on shutdown: chunks
            // still resident then never hit the unload path that would persist them.
            asio::awaitable<bool> persistAll();

        private:
            asio::awaitable<void> _unloadChunk(const proto::file::ChunkID& id);
            asio::awaitable<bool> _persistChunkIfDirty(const proto::file::ChunkID& id);

            std::unique_ptr<file::IWorldFile> _archive;
            AssetCache<proto::file::ChunkID, proto::file::WorldChunk> _cache;

            float _chunk_size;

            // archive/disk is newer than the cached chunk. 
            // write(chunk) writes to the archive, then marks the loaded chunk for reload.
            // Next updateLoadPosition() re-reads it into _cache and clears _to_reload.
            absl::flat_hash_set<proto::file::ChunkID> _to_reload;
            absl::flat_hash_set<proto::file::ChunkID> _required_chunks;

            //_dirty_chunks: cached chunk is newer than archive/disk.
            // Before unloading, _persistChunkIfDirty() would write the cached chunk back to the archive,
            // then clear _dirty_chunks
            absl::flat_hash_set<proto::file::ChunkID> _dirty_chunks;
    };
}
