#include "file/save_archive.hpp"

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
    SaveArchive<use_binary_t>::SaveArchive(std::filesystem::path file_path)
        : _file_path(std::move(file_path))
    {
        _openStream(std::ios::in | std::ios::binary);

        if (!_stream.is_open() || !_stream.good())
            return;

        // need to load SaveArchiveData from binary file 
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

            WorldChunk chunk;
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
    }

    bool SaveArchive<use_binary_t>::_openStream(std::ios::openmode mode) {
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


    bool SaveArchive<use_binary_t>::_closeStream()
    {
        _stream.close();
        return !_stream.is_open();
    }

    const absl::flat_hash_set<ChunkID> & SaveArchive<use_binary_t>::getAllChunks() const
    {
        return _all_chunks;
    }

    static std::optional<std::size_t> _calculateChunkByteSize(const WorldChunk & chunk)
    {
        std::ostringstream buffer_stream(std::ios::binary);

        google::protobuf::io::OstreamOutputStream raw(&buffer_stream);
        google::protobuf::io::CodedOutputStream coded_output(&raw);

        // Serialize chunk to memory to calculate size requirements
        if (!google::protobuf::util::SerializeDelimitedToOstream(chunk, &buffer_stream))
        {
            spdlog::error("Failed to serialize chunk to buffer");
            return std::nullopt;
        }

        return coded_output.ByteCount();
    }

    bool SaveArchive<use_binary_t>::writeChunk(const WorldChunk & chunk)
    {
        if (!_openStream(std::ios::in | std::ios::out | std::ios::binary))
        {
            spdlog::error("Failed to open stream for writing");
            return false;
        }

        auto exising_chunk = readChunk(chunk.id());
        if(exising_chunk)
        {   
            const auto & chunk_index = _chunk_index[chunk.id()];
            
            const auto new_size_res = _calculateChunkByteSize(*exising_chunk);
            if(!new_size_res)
            {
                spdlog::error("Failed to calculate chunk size while updating in place");
                return false;
            }

            const std::size_t & new_size = new_size_res.value();
            
            // check whether we can replace in place
            if (new_size <= chunk_index.size)
            {
                // Replace in-place
                _stream.seekp(chunk_index.offset);

                { // serialize using coded stream
                    google::protobuf::io::OstreamOutputStream raw(&_stream);
                    google::protobuf::io::CodedOutputStream coded_output(&raw);
                    // save chunk to stream in binary format
                    if (!google::protobuf::util::SerializeDelimitedToCodedStream(chunk, &coded_output))
                    {
                        spdlog::error("Failed to serialize chunk to stream");
                        _stream.close();
                        return false;
                    }

                    // TODO zero padding
                }

                // chunk updated in place
                _stream.close();
                return true;
            }
        }

        // append at the end
        _stream.seekp(0, std::ios::end);
        
        // save put position before writing
        std::streamoff offset = _stream.tellp();

        { // serialize using coded stream
            google::protobuf::io::OstreamOutputStream raw(&_stream);
            google::protobuf::io::CodedOutputStream coded_output(&raw);

            // save chunk to stream in binary format
            if (!google::protobuf::util::SerializeDelimitedToCodedStream(chunk, &coded_output))
            {
                spdlog::error("Failed to serialize chunk to stream");
                _stream.close();
                return false;
            }

            // update logical index
            _chunk_index[chunk.id()].offset = offset;
            _chunk_index[chunk.id()].size = coded_output.ByteCount() - offset;
        }

        _all_chunks.emplace(chunk.id());

        _stream.close();
        return true;
    }

    std::optional<WorldChunk> SaveArchive<use_binary_t>::readChunk(const ChunkID & id)
    {
        if (!_chunk_index.contains(id)) 
        {
            return std::nullopt;
        }

        if (!_openStream(std::ios::in | std::ios::binary))
        {
            spdlog::error("Failed to open stream for reading");
            return std::nullopt;
        }

        // in binary format we need to seek to the offset
        const auto offset = _chunk_index[id].offset;
        _stream.seekg(offset);

        google::protobuf::io::IstreamInputStream raw(&_stream);
        google::protobuf::io::CodedInputStream coded_input(&raw);

        // read message size
        uint32_t message_size = 0;
        if (!coded_input.ReadVarint32(&message_size))
        {
            spdlog::error("Failed to read size prefix");
            _stream.close();
            return std::nullopt;
        }

        assert(message_size == _chunk_index[id].size && "Chunk size mismatch");

        // Limit reading to current message size
        google::protobuf::io::CodedInputStream::Limit limit = coded_input.PushLimit(message_size);

        // read chunk
        WorldChunk result;
        if (!result.ParseFromCodedStream(&coded_input))
        {
            spdlog::error("Failed to parse chunk");
            _stream.close();
            return std::nullopt;
        }

        coded_input.PopLimit(limit);
        _stream.close();

        return result;
    }

    bool SaveArchive<use_binary_t>::removeChunk(const ChunkID& id)
    {
        if (!_chunk_index.contains(id))
        {
            return false;
        }

        if (!_openStream(std::ios::in | std::ios::out | std::ios::binary))
        {
            spdlog::error("Failed to open stream for writing");
            return false;
        }

        // in binary format we need to seek to the offset
        const auto offset = _chunk_index[id].offset;
        _stream.seekp(offset);

        
    }

    bool SaveArchive<use_binary_t>::writeEntity(const ChunkID& chunk_id,
                                  const ecs::EntityDefinition& entity_def)
    {
        // Load existing chunk (binary)
        WorldChunk chunk;
        if (auto existing = readChunk(chunk_id))
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
        const std::size_t & entity_id = entity_def.id();

        auto it = std::find_if(entities->begin(), entities->end(),
            [&](const ecs::EntityDefinition& e) {
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

        // Persist updated chunk via existing logic (handles in-place/append & index)
        return writeChunk(chunk);
    }

    bool SaveArchive<use_binary_t>::removeEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def)
    {
        return false;
    }
}
