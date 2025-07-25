#pragma once
#include <utility>
#include <filesystem>
#include <fstream>
#include <optional>

#include <absl/container/flat_hash_map.h>

#include "ecs/ecs.hpp"
#include "asset/asset.hpp"
#include "world/chunk.hpp"

namespace astre::world
{
    struct ChunkIndexEntry {
        std::uint64_t offset;
        std::uint64_t size;
    };

    class SaveArchive
    {
    public:
        SaveArchive(const std::filesystem::path& file_path);

        bool loadIndex();
        bool saveIndex();

        bool writeChunk(const ChunkID& id,
                const std::vector<ecs::EntityDefinition>& entities,
                asset::use_binary_t);

        bool writeChunk(const ChunkID& id,
                const std::vector<ecs::EntityDefinition>& entities,
                asset::use_json_t);

        std::optional<std::vector<ecs::EntityDefinition>> readChunk(const ChunkID& id, asset::use_binary_t);
        std::optional<std::vector<ecs::EntityDefinition>> readChunk(const ChunkID& id, asset::use_json_t);

    private:
        std::filesystem::path _file_path;
        std::fstream _stream;
        absl::flat_hash_map<ChunkID, ChunkIndexEntry> _chunk_index;

        bool openStream(std::ios::openmode mode);
    };
}