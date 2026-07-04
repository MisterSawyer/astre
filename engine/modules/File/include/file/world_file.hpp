#pragma once
#include <utility>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "type/type.hpp"

#include "file/data_type.hpp"

#include "proto/ECS/entity_definition.pb.h"
#include "proto/File/world_chunk.pb.h"
#include "proto/File/world_file_data.pb.h"

namespace astre::proto::file
{
    inline bool operator==(const ChunkID& lhs, const ChunkID& rhs)
    { return lhs.x() == rhs.x() && lhs.y() == rhs.y() && lhs.z() == rhs.z(); }


    inline bool operator==(const WorldChunk& lhs, const WorldChunk& rhs)
    { return lhs.id() == rhs.id(); }

    template <typename H>
    inline H AbslHashValue(H h, const ChunkID & id) {
        return H::combine(std::move(h), id.x(), id.y(), id.z());
    }

    template <typename H>
    inline H AbslHashValue(H h, const WorldChunk & world_chunk) {
        return H::combine(std::move(h), world_chunk.id());
    }
}

namespace astre::file
{
    struct ChunkIndexEntry
    {
        std::size_t index;

        std::streamoff offset;
        std::size_t size;
    };
    
    class IWorldFile : type::InterfaceBase
    {
        public:
            virtual ~IWorldFile() = default;

            virtual bool writeChunk(const proto::file::WorldChunk & chunk) = 0;

            // Source-read shape (like file::ShaderFile/ScriptFile::read): loads one
            // chunk off disk by id. const + uses a local stream per call, so
            // concurrent reads are safe — that is what lets asset::WorldStreamer
            // fan chunk loads over the pool through asset::streamAssets.
            // reads are concurrent-safe among themselves; a concurrent
            // write()/remove() (editor) still mutates the index unsynchronized —
            // fine today (streaming and edits don't overlap), revisit if they do.
            virtual std::optional<proto::file::WorldChunk> read(const proto::file::ChunkID& id) const = 0;

            virtual bool removeChunk(const proto::file::ChunkID& id) = 0;

            virtual const absl::flat_hash_set<proto::file::ChunkID> & getAllChunks() const = 0;


            virtual bool writeEntity(const proto::file::ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) = 0;

            virtual bool removeEntity(const proto::file::ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) = 0;
    };

    template<class Mode>
    class WorldFile;

    template<>
    class WorldFile<use_binary_t> : public IWorldFile
    {
        public:
            WorldFile(std::filesystem::path file_path);

            // whether the archive opened + scanned successfully at construction
            bool good() const { return _valid; }

            bool writeChunk(const proto::file::WorldChunk & chunk) override;
            std::optional<proto::file::WorldChunk> read(const proto::file::ChunkID& id) const override;
            bool removeChunk(const proto::file::ChunkID& id) override;
            const absl::flat_hash_set<proto::file::ChunkID> & getAllChunks() const override;

            bool writeEntity(const proto::file::ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) override;
            bool removeEntity(const proto::file::ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) override;

    private:
        bool _openStream(std::ios::openmode mode);
        bool _closeStream();

        // Whole-file read/rewrite. The binary format is a flat stream of delimited
        // chunks, so any mutation (write/remove) rewrites the file and rebuilds the
        // offset index — the same all-or-nothing approach WorldFile<use_json_t> uses.
        std::vector<proto::file::WorldChunk> _readAllChunks() const;
        bool _writeAllChunks(const std::vector<proto::file::WorldChunk> & chunks);

        std::filesystem::path _file_path;
        std::fstream _stream;
        bool _valid = false;
        absl::flat_hash_set<proto::file::ChunkID> _all_chunks;
        absl::flat_hash_map<proto::file::ChunkID, ChunkIndexEntry> _chunk_index;
    };


    template<>
    class WorldFile<use_json_t> : public IWorldFile
    {
        public:
            WorldFile(std::filesystem::path file_path);

            // whether the archive opened + parsed successfully at construction
            bool good() const { return _valid; }

            bool writeChunk(const proto::file::WorldChunk & chunk) override;
            std::optional<proto::file::WorldChunk> read(const proto::file::ChunkID& id) const override;
            bool removeChunk(const proto::file::ChunkID& id) override;
            const absl::flat_hash_set<proto::file::ChunkID> & getAllChunks() const override;

            bool writeEntity(const proto::file::ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) override;
            bool removeEntity(const proto::file::ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) override;

    private:
        bool _openStream(std::ios::openmode mode);
        bool _closeStream();
        std::stringstream _readStream() const;

        std::filesystem::path _file_path;
        std::fstream _stream;
        bool _valid = false;
        absl::flat_hash_set<proto::file::ChunkID> _all_chunks;
        absl::flat_hash_map<proto::file::ChunkID, ChunkIndexEntry> _chunk_index;
    };

    // Stage-1 archive factory: picks the on-disk format from the mode tag. The
    // world analogue of ShaderFile/ScriptFile binding a directory — the disk
    // source is injected into asset::WorldStreamer, not hardcoded from a path.
    // Returns nullptr if the file could not be opened/parsed, so the caller can
    // detect a failed open.
    inline std::unique_ptr<IWorldFile> openWorldArchive(std::filesystem::path path, use_json_t)
    {
        auto archive = std::make_unique<WorldFile<use_json_t>>(std::move(path));
        if(!archive->good()) return nullptr;
        return archive;
    }

    inline std::unique_ptr<IWorldFile> openWorldArchive(std::filesystem::path path, use_binary_t)
    {
        auto archive = std::make_unique<WorldFile<use_binary_t>>(std::move(path));
        if(!archive->good()) return nullptr;
        return archive;
    }
}