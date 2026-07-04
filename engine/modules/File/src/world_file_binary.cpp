#include "file/world_file.hpp"

#include <algorithm>

#include <google/protobuf/util/delimited_message_util.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

namespace astre::file 
{
    static constexpr std::int32_t MAGIC = 0xABCD1234;

    // layout of binary file
    //<varint_size><chunk_0_bytes>
    //<varint_size><chunk_1_bytes>
    //...
    WorldFile<use_binary_t>::WorldFile(std::filesystem::path file_path)
        : _file_path(std::move(file_path))
    {
        _openStream(std::ios::in | std::ios::binary);

        if (!_stream.is_open() || !_stream.good())
            return;

        // need to load WorldFileData from binary file 
        // then scan it and add all indexes, and their offsets and sizes
        // such that we  then can read from file only given part
        google::protobuf::io::IstreamInputStream raw(&_stream);
        google::protobuf::io::CodedInputStream coded_input(&raw);

        while (true)
        {
            std::streamoff offset = coded_input.CurrentPosition();

            uint32_t message_size = 0;
            if (!coded_input.ReadVarint32(&message_size))
                break;  // EOF

            // Limit reading to current message size
            auto limit = coded_input.PushLimit(message_size);

            proto::file::WorldChunk chunk;
            if (!chunk.ParseFromCodedStream(&coded_input))
            {
                spdlog::warn("Failed to parse chunk at offset {}", offset);
                break;
            }

            coded_input.PopLimit(limit);

            // Build logical index: ChunkID → index
            // such that we can read from file only given part
            // we need to store offset as absolute offset from the beginning of the file

            _chunk_index[chunk.id()].offset = offset;
            _chunk_index[chunk.id()].size = message_size;

            _all_chunks.emplace(chunk.id());
        }

        _stream.close();
        _valid = true;
    }

    bool WorldFile<use_binary_t>::_openStream(std::ios::openmode mode) {
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


    bool WorldFile<use_binary_t>::_closeStream()
    {
        _stream.close();
        return !_stream.is_open();
    }

    const absl::flat_hash_set<proto::file::ChunkID> & WorldFile<use_binary_t>::getAllChunks() const
    {
        return _all_chunks;
    }

    std::vector<proto::file::WorldChunk> WorldFile<use_binary_t>::_readAllChunks() const
    {
        std::vector<proto::file::WorldChunk> chunks;

        std::ifstream stream(_file_path, std::ios::in | std::ios::binary);
        if (!stream.is_open())
            return chunks;

        google::protobuf::io::IstreamInputStream raw(&stream);
        google::protobuf::io::CodedInputStream coded_input(&raw);

        while (true)
        {
            uint32_t message_size = 0;
            if (!coded_input.ReadVarint32(&message_size))
                break;  // EOF

            auto limit = coded_input.PushLimit(message_size);

            proto::file::WorldChunk chunk;
            if (!chunk.ParseFromCodedStream(&coded_input))
            {
                spdlog::warn("[world-file] Failed to parse chunk while reading all");
                break;
            }

            coded_input.PopLimit(limit);
            chunks.push_back(std::move(chunk));
        }

        return chunks;
    }

    bool WorldFile<use_binary_t>::_writeAllChunks(const std::vector<proto::file::WorldChunk> & chunks)
    {
        if (!_openStream(std::ios::out | std::ios::trunc | std::ios::binary))
        {
            spdlog::error("[world-file] Failed to open stream for writing");
            return false;
        }

        _chunk_index.clear();
        _all_chunks.clear();

        {
            google::protobuf::io::OstreamOutputStream raw(&_stream);
            google::protobuf::io::CodedOutputStream coded_output(&raw);

            for (const auto & chunk : chunks)
            {
                // ByteCount() is the absolute file offset here: we write
                // sequentially into a freshly-truncated stream. It marks the start
                // of the varint length prefix, matching the constructor's index.
                const std::streamoff offset = coded_output.ByteCount();
                const std::size_t payload_size = chunk.ByteSizeLong();

                if (!google::protobuf::util::SerializeDelimitedToCodedStream(chunk, &coded_output))
                {
                    spdlog::error("[world-file] Failed to serialize chunk to stream");
                    _closeStream();
                    return false;
                }

                _chunk_index[chunk.id()] = ChunkIndexEntry{0, offset, payload_size};
                _all_chunks.emplace(chunk.id());
            }
        }

        _closeStream();
        return true;
    }

    bool WorldFile<use_binary_t>::writeChunk(const proto::file::WorldChunk & chunk)
    {
        std::vector<proto::file::WorldChunk> chunks = _readAllChunks();

        bool replaced = false;
        for (auto & c : chunks)
        {
            if (c.id() == chunk.id())
            {
                c = chunk;
                replaced = true;
                break;
            }
        }
        if (!replaced)
            chunks.push_back(chunk);

        return _writeAllChunks(chunks);
    }

    std::optional<proto::file::WorldChunk> WorldFile<use_binary_t>::read(const proto::file::ChunkID & id) const
    {
        const auto index_it = _chunk_index.find(id);
        if (index_it == _chunk_index.end())
        {
            return std::nullopt;
        }

        // local stream (not the member _stream): const + safe to call concurrently.
        std::ifstream stream(_file_path, std::ios::in | std::ios::binary);
        if (!stream.is_open())
        {
            spdlog::error("Failed to open stream for reading");
            return std::nullopt;
        }

        // in binary format we need to seek to the offset
        stream.seekg(index_it->second.offset);

        google::protobuf::io::IstreamInputStream raw(&stream);
        google::protobuf::io::CodedInputStream coded_input(&raw);

        // read message size
        uint32_t message_size = 0;
        if (!coded_input.ReadVarint32(&message_size))
        {
            spdlog::error("Failed to read size prefix");
            return std::nullopt;
        }

        assert(message_size == index_it->second.size && "Chunk size mismatch");

        // Limit reading to current message size
        google::protobuf::io::CodedInputStream::Limit limit = coded_input.PushLimit(message_size);

        // read chunk
        proto::file::WorldChunk result;
        if (!result.ParseFromCodedStream(&coded_input))
        {
            spdlog::error("Failed to parse chunk");
            return std::nullopt;
        }

        coded_input.PopLimit(limit);

        return result;
    }

    bool WorldFile<use_binary_t>::removeChunk(const proto::file::ChunkID& id)
    {
        if (!_chunk_index.contains(id))
        {
            return false;
        }

        std::vector<proto::file::WorldChunk> chunks = _readAllChunks();

        const auto new_end = std::remove_if(chunks.begin(), chunks.end(),
            [&](const proto::file::WorldChunk & c){ return c.id() == id; });
        if (new_end == chunks.end())
        {
            spdlog::warn("[world-file] chunk present in index but not in file");
            return false;
        }
        chunks.erase(new_end, chunks.end());

        return _writeAllChunks(chunks);
    }

    bool WorldFile<use_binary_t>::writeEntity(const proto::file::ChunkID& chunk_id,
                                  const proto::ecs::EntityDefinition& entity_def)
    {
        // Load existing chunk (binary)
        proto::file::WorldChunk chunk;
        if (auto existing = read(chunk_id))
        {
            chunk = std::move(*existing);
        }
        else
        {
            spdlog::error("Failed to find chunk to update");
            return false;
        }

        // Replace or append entity by id()
        auto* entities = chunk.mutable_entities();
        const auto entity_id = entity_def.id();

        auto it = std::find_if(entities->begin(), entities->end(),
            [&](const proto::ecs::EntityDefinition& e) {
                return e.id() == entity_id;
            }
        );

        if (it != entities->end())
        {
            it->CopyFrom(entity_def);
        }
        else
        {
            entities->Add()->CopyFrom(entity_def);
        }

        // Persist updated chunk via existing logic (handles rewrite & index)
        return writeChunk(chunk);
    }

    bool WorldFile<use_binary_t>::removeEntity(const proto::file::ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def)
    {
        auto existing = read(chunk_id);
        if (!existing)
        {
            spdlog::warn("[world-file] removeEntity: chunk not found");
            return false;
        }

        auto* entities = existing->mutable_entities();
        const auto entity_id = entity_def.id();

        for (int i = 0; i < entities->size(); ++i)
        {
            if (entities->Get(i).id() == entity_id)
            {
                entities->DeleteSubrange(i, 1);
                return writeChunk(*existing);
            }
        }

        spdlog::warn("[world-file] removeEntity: entity '{}' not found in chunk", entity_id);
        return false;
    }
}
