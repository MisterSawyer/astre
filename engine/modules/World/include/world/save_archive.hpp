#pragma once
#include <utility>
#include <filesystem>
#include <fstream>
#include <optional>

#include <absl/container/flat_hash_map.h>

#include "ecs/ecs.hpp"
#include "asset/asset.hpp"

#include "generated/World/proto/world_chunk.pb.h"
#include "generated/World/proto/save_archive_data.pb.h"

namespace astre::world
{
    inline bool operator==(const ChunkID& lhs, const ChunkID& rhs)
    { return lhs.x() == rhs.x() && lhs.y() == rhs.y() && lhs.z() == rhs.z(); }

    template <typename H>
    inline H AbslHashValue(H h, const ChunkID & id) {
        return H::combine(std::move(h), id.x(), id.y(), id.z());
    }

    struct ChunkIndexEntry
    {
        std::size_t index;

        std::streamoff offset;
        std::size_t size;
    };

    class SaveArchive
    {
    public:
        SaveArchive(std::filesystem::path file_path, asset::use_json_t);
        SaveArchive(std::filesystem::path file_path, asset::use_binary_t);

        bool writeChunk(const WorldChunk & chunk, asset::use_binary_t);
        bool writeChunk(const WorldChunk & chunk, asset::use_json_t);

        std::optional<WorldChunk> readChunk(const ChunkID& id, asset::use_binary_t);
        std::optional<WorldChunk> readChunk(const ChunkID& id, asset::use_json_t);

    private:

        std::filesystem::path _file_path;
        std::fstream _stream;
        absl::flat_hash_map<ChunkID, ChunkIndexEntry> _chunk_index;

        bool openStream(std::ios::openmode mode);
    };
}