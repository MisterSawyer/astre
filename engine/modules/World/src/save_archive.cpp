#include "world/save_archive.hpp"

#include <google/protobuf/util/delimited_message_util.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

namespace astre::world 
{
    constexpr static const std::int32_t MAGIC = 0xABCD1234;

    SaveArchive::SaveArchive(const std::filesystem::path& file_path)
        : _file_path(file_path)
    {}

    bool SaveArchive::openStream(std::ios::openmode mode) {
        if (_stream.is_open()) {
            _stream.close();  // Ensure no existing stream is active
        }

        _stream.clear(); // Reset error flags (e.g. after EOF)

        if (!std::filesystem::exists(_file_path))
        {
            // Create file by opening with out only
            std::ofstream create(_file_path);
            if (!create) 
            {
                spdlog::error("Failed to create file");
                return false;
            }
        }

        _stream.open(_file_path, mode);

        return _stream.is_open() && _stream.good();
    }

    bool SaveArchive::writeChunk(const ChunkID& id,
                                 const std::vector<ecs::EntityDefinition>& entities,
                                 asset::use_binary_t)
    {
        if (!openStream(std::ios::in | std::ios::out | std::ios::app | std::ios::binary))
        {
            spdlog::error("Failed to open stream for writing");
            return false;
        }

        _stream.seekp(0, std::ios::end);
        std::uint64_t offset = _stream.tellp();

        for (const auto& entity : entities) {
            google::protobuf::util::SerializeDelimitedToOstream(entity, &_stream);
        }

        auto current = static_cast<std::streamoff>(_stream.tellp());
        auto start = static_cast<std::streamoff>(offset);
        std::uint64_t size = static_cast<std::uint64_t>(current - start);

        _chunk_index[id] = { offset, size };
        _stream.close();
        return true;
    }

    bool SaveArchive::writeChunk(const ChunkID& id,
                                 const std::vector<ecs::EntityDefinition>& entities,
                                 asset::use_json_t) 
    {
        if (!openStream(std::ios::in | std::ios::out | std::ios::app))
        {
            spdlog::error("Failed to open stream for writing");
            return false;
        }

        _stream.seekp(0, std::ios::end);
        std::uint64_t offset = _stream.tellp();

        for (const auto& entity : entities) {
            std::string json;
            google::protobuf::util::JsonPrintOptions options;
            options.add_whitespace = true;  // pretty-print
            options.always_print_fields_with_no_presence = true; // ensures e.g. `health: 0` shows
            options.preserve_proto_field_names = true; // ensures field names are not converted to camelCase

            google::protobuf::util::MessageToJsonString(entity, &json, options);
            _stream.write(json.data(), json.size());
        }

        auto current = static_cast<std::streamoff>(_stream.tellp());
        auto start = static_cast<std::streamoff>(offset);
        std::uint64_t size = static_cast<std::uint64_t>(current - start);

        _chunk_index[id] = { offset, size };
        _stream.close();
        return true;
    }

    std::optional<std::vector<ecs::EntityDefinition>>
    SaveArchive::readChunk(const ChunkID & id, asset::use_binary_t)
    {
        if (!_chunk_index.contains(id)) 
        {
            spdlog::error("Chunk not found");
            return std::nullopt;
        }

        if (!openStream(std::ios::in))
        {
            spdlog::error("Failed to open stream for reading");
            return std::nullopt;
        }

        const auto [offset, size] = _chunk_index[id];
        _stream.seekg(offset);

        std::vector<ecs::EntityDefinition> result;
        google::protobuf::io::IstreamInputStream zero(&_stream);

        while (_stream.tellg() < offset + static_cast<std::streamoff>(size)) {
            ecs::EntityDefinition def;
            bool clean_eof = false;

            if (google::protobuf::util::ParseDelimitedFromZeroCopyStream(&def, &zero, &clean_eof)) {
                result.push_back(std::move(def));
            } else {
                break;
            }
        }

        _stream.close();
        return result;
    }

    std::optional<std::vector<ecs::EntityDefinition>>
    SaveArchive::readChunk(const ChunkID & id, asset::use_json_t)
    {
        if (!_chunk_index.contains(id))
        {
            spdlog::error("Chunk not found");
            return std::nullopt;
        }

        if (!openStream(std::ios::in)) 
        {
            spdlog::error("Failed to open stream for reading");
            return std::nullopt;
        }

        const auto [offset, size] = _chunk_index[id];
        _stream.seekg(offset);

        std::vector<ecs::EntityDefinition> result;
        const auto end_offset = offset + static_cast<std::streamoff>(size);

        while (_stream.tellg() < end_offset) {
            uint32_t len = 0;
            _stream.read(reinterpret_cast<char*>(&len), sizeof(len));
            if (_stream.eof() || len == 0) break;

            std::string json(len, '\0');
            _stream.read(json.data(), len);

            ecs::EntityDefinition def;
            google::protobuf::util::JsonParseOptions options;
            options.ignore_unknown_fields = true;

            auto status = google::protobuf::util::JsonStringToMessage(json, &def, options);
            if (status.ok()) {
                result.push_back(std::move(def));
            } else {
                break;
            }
        }

        _stream.close();
        return result;
    }

    bool SaveArchive::loadIndex()
    {
        if (!openStream(std::ios::in))
        {
            spdlog::error("Failed to open stream for reading");
            return false;
        }

        _stream.seekg(0, std::ios::end);
        std::streampos end = _stream.tellg();

        // Try to find the footer from the end
        constexpr std::size_t footer_size = sizeof(uint32_t);
        constexpr std::size_t entry_size = sizeof(int32_t) * 2 + sizeof(uint64_t) * 2;

        _stream.seekg(end - static_cast<std::streamoff>(footer_size), std::ios::beg);
        uint32_t magic_end = 0;
        _stream.read(reinterpret_cast<char*>(&magic_end), sizeof(magic_end));

        if (magic_end != MAGIC)
        {
            spdlog::error("Invalid file format");
            _stream.close();
            return false; // Invalid file format
        }

        // Walk backward to find index start
        std::streamoff cursor = end - static_cast<std::streamoff>(footer_size);

        // Step back to read count
        cursor -= sizeof(uint32_t);
        _stream.seekg(cursor);
        uint32_t count = 0;
        _stream.read(reinterpret_cast<char*>(&count), sizeof(count));

        cursor -= count * entry_size;
        cursor -= sizeof(uint32_t); // initial magic

        _stream.seekg(cursor);
        uint32_t magic_start = 0;
        _stream.read(reinterpret_cast<char*>(&magic_start), sizeof(magic_start));
        if (magic_start != MAGIC)
        {
            spdlog::error("Invalid file format");
            _stream.close();
            return false;
        }

        // Read entries
        for (uint32_t i = 0; i < count; ++i) {
            int32_t x = 0, y = 0;
            uint64_t offset = 0, size = 0;

            _stream.read(reinterpret_cast<char*>(&x), sizeof(x));
            _stream.read(reinterpret_cast<char*>(&y), sizeof(y));
            _stream.read(reinterpret_cast<char*>(&offset), sizeof(offset));
            _stream.read(reinterpret_cast<char*>(&size), sizeof(size));

            _chunk_index[{x, y}] = { offset, size };
        }

        _stream.close();
        return true;
    }

    
    bool SaveArchive::saveIndex()
    {
        if (!openStream(std::ios::in | std::ios::out | std::ios::app))
        {
            spdlog::error("Failed to open stream for writing");
            return false;
        }

        uint32_t count = static_cast<uint32_t>(_chunk_index.size());

        _stream.write(reinterpret_cast<const char*>(&MAGIC), sizeof(MAGIC));
        _stream.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& [chunk, entry] : _chunk_index) 
        {
            _stream.write(reinterpret_cast<const char*>(&chunk.x), sizeof(chunk.x));
            _stream.write(reinterpret_cast<const char*>(&chunk.y), sizeof(chunk.y));
            _stream.write(reinterpret_cast<const char*>(&entry.offset), sizeof(entry.offset));
            _stream.write(reinterpret_cast<const char*>(&entry.size), sizeof(entry.size));
        }

        _stream.write(reinterpret_cast<const char*>(&MAGIC), sizeof(MAGIC));
        _stream.close();
        return true;
    }

}
