#include "world/save_archive.hpp"

#include <google/protobuf/util/delimited_message_util.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

namespace astre::world 
{
    constexpr static const std::int32_t MAGIC = 0xABCD1234;

    // layout of json file
    //chunks: [
    //    {chunk0}, {chunk1}, ...
    //]
    SaveArchive::SaveArchive(std::filesystem::path file_path, asset::use_json_t)
        : _file_path(std::move(file_path))
    {
        openStream(std::ios::in);

        if(!_stream.is_open() || !_stream.good())return;

        // need to load SaveArchiveData from json file 
        // then scan it and add all indexes, and their offsets and sizes
        // such that we  then can read from file only given part
        std::stringstream buffer;
        buffer << _stream.rdbuf();
        _stream.close();

        SaveArchiveData data;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = true;

        auto status = google::protobuf::util::JsonStringToMessage(buffer.str(), &data, options);
        if (!status.ok())
        {
            spdlog::error("Failed to parse SaveArchiveData JSON: {}", status.ToString());
            return;
        }

        // Build logical index: ChunkID → index
        for (std::size_t i = 0; i < static_cast<std::size_t>(data.chunks_size()); ++i)
        {
            const auto& chunk = data.chunks(static_cast<int>(i));
            _chunk_index[chunk.id()].index = i;
        }
    }

    // layout of binary file
    //<varint_size><chunk_0_bytes>
    //<varint_size><chunk_1_bytes>
    //...
    SaveArchive::SaveArchive(std::filesystem::path file_path, asset::use_binary_t)
        : _file_path(std::move(file_path))
    {
        openStream(std::ios::in | std::ios::binary);

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
        }

        _stream.close();
    }

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

    bool SaveArchive::writeChunk(const WorldChunk & chunk, asset::use_binary_t)
    {
        if (!openStream(std::ios::in | std::ios::out | std::ios::binary))
        {
            spdlog::error("Failed to open stream for writing");
            return false;
        }

        auto exising_chunk = readChunk(chunk.id(), asset::use_binary);
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

        _stream.close();
        return true;
    }

    bool SaveArchive::writeChunk(const WorldChunk & chunk, asset::use_json_t) 
    {
        // Load existing
        SaveArchiveData archive;

        if (std::ifstream input(_file_path); input.good())
        {
            std::stringstream buffer;
            buffer << input.rdbuf();
            google::protobuf::util::JsonStringToMessage(buffer.str(), &archive);
        }

        // Replace or append
        bool replaced = false;
        for (auto& c : *archive.mutable_chunks())
        {
            if (c.id() == chunk.id())
            {
                // replace
                c.CopyFrom(chunk);
                replaced = true;
                break;
            }
        }

        if (!replaced)
        {
            // append
            archive.add_chunks()->CopyFrom(chunk);
        }

        // Write back
        std::string json;
        google::protobuf::util::JsonPrintOptions options;
        options.preserve_proto_field_names = true;
        options.add_whitespace = true;

        google::protobuf::util::MessageToJsonString(archive, &json, options);

        std::ofstream out(_file_path);
        if (!out)
        {
            spdlog::error("Failed to open stream for writing JSON");
            return false;
        }

        out << json;
        return true;
    }

    std::optional<WorldChunk> SaveArchive::readChunk(const ChunkID & id, asset::use_binary_t)
    {
        if (!_chunk_index.contains(id)) 
        {
            return std::nullopt;
        }

        if (!openStream(std::ios::in | std::ios::binary))
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

    std::optional<WorldChunk> SaveArchive::readChunk(const ChunkID & id, asset::use_json_t)
    {
        if (!_chunk_index.contains(id))
        {
            return std::nullopt;
        }

        if (!openStream(std::ios::in)) 
        {
            spdlog::error("Failed to open stream for reading");
            return std::nullopt;
        }

        // we need to read whole message to parse chunks array
        std::stringstream buffer;
        buffer << _stream.rdbuf();
        _stream.close();

        SaveArchiveData archive;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = true;

        // parse archive data
        auto status = google::protobuf::util::JsonStringToMessage(buffer.str(), &archive, options);
        if (!status.ok())
        {
            spdlog::error("Failed to parse SaveArchiveData JSON: {}", status.ToString());
            return std::nullopt;
        }

        const std::size_t index = _chunk_index[id].index;
        if (index >= static_cast<std::size_t>(archive.chunks_size()))
        {
            spdlog::error("Invalid index in chunk map");
            return std::nullopt;
        }

        // return chunk from array
        return archive.chunks(static_cast<int>(index));
    }
}
