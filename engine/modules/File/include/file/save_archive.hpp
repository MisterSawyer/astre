#pragma once
#include <utility>
#include <filesystem>
#include <fstream>
#include <optional>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "type/type.hpp"

#include "file/data_type.hpp"

#include "proto/ECS/entity_definition.pb.h"
#include "proto/File/world_chunk.pb.h"
#include "proto/File/save_archive_data.pb.h"

namespace astre::file
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

    struct ChunkIndexEntry
    {
        std::size_t index;

        std::streamoff offset;
        std::size_t size;
    };
    
    class ISaveArchive : type::InterfaceBase
    {
        public:
            virtual ~ISaveArchive() = default;

            virtual bool writeChunk(const WorldChunk & chunk) = 0;

            virtual std::optional<WorldChunk> readChunk(const ChunkID& id) = 0;

            virtual bool removeChunk(const ChunkID& id) = 0;

            virtual const absl::flat_hash_set<ChunkID> & getAllChunks() const = 0;


            virtual bool writeEntity(const ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) = 0;

            virtual bool removeEntity(const ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) = 0;
    };

    template<class Mode>
    class SaveArchive;

    template<>
    class SaveArchive<use_binary_t> : public ISaveArchive
    {
        public:
            SaveArchive(std::filesystem::path file_path);

            bool writeChunk(const WorldChunk & chunk) override;
            std::optional<WorldChunk> readChunk(const ChunkID& id) override;
            bool removeChunk(const ChunkID& id) override;
            const absl::flat_hash_set<ChunkID> & getAllChunks() const override;

            bool writeEntity(const ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) override;
            bool removeEntity(const ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) override;

    private:
        bool _openStream(std::ios::openmode mode);
        bool _closeStream();

        std::filesystem::path _file_path;
        std::fstream _stream;
        absl::flat_hash_set<ChunkID> _all_chunks;
        absl::flat_hash_map<ChunkID, ChunkIndexEntry> _chunk_index;
    };


    template<>
    class SaveArchive<use_json_t> : public ISaveArchive
    {
        public:
            SaveArchive(std::filesystem::path file_path);

            bool writeChunk(const WorldChunk & chunk) override;
            std::optional<WorldChunk> readChunk(const ChunkID& id) override;
            bool removeChunk(const ChunkID& id) override;
            const absl::flat_hash_set<ChunkID> & getAllChunks() const override;

            bool writeEntity(const ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) override;
            bool removeEntity(const ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def) override;
            
    private:
        bool _openStream(std::ios::openmode mode);
        bool _closeStream();
        std::stringstream _readStream() const;

        std::filesystem::path _file_path;
        std::fstream _stream;
        absl::flat_hash_set<ChunkID> _all_chunks;
        absl::flat_hash_map<ChunkID, ChunkIndexEntry> _chunk_index;
    };
}