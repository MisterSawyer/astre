#include "world/save_archive.hpp"

#include <google/protobuf/util/delimited_message_util.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

namespace astre::world 
{

    // layout of json file
    //chunks: [
    //    {chunk0}, {chunk1}, ...
    //]
    SaveArchive<asset::use_json_t>::SaveArchive(std::filesystem::path file_path)
        : _file_path(std::move(file_path))
    {
        if(!_openStream(std::ios::in))return;

        // need to load SaveArchiveData from json file 
        // then scan it and add all indexes, and their offsets and sizes
        // such that we  then can read from file only given part
        std::stringstream buffer = _readStream();
        _closeStream();

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
            _all_chunks.emplace(chunk.id());
        }
    }


    bool SaveArchive<asset::use_json_t>::_openStream(std::ios::openmode mode) {
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

    bool SaveArchive<asset::use_json_t>::_closeStream()
    {
        _stream.close();
        return !_stream.is_open();
    }

    std::stringstream SaveArchive<asset::use_json_t>::_readStream() const
    {
        std::stringstream buffer;
        buffer << _stream.rdbuf();
        return buffer;
    }

    const absl::flat_hash_set<ChunkID> & SaveArchive<asset::use_json_t>::getAllChunks() const
    {
        return _all_chunks;
    }

    bool SaveArchive<asset::use_json_t>::writeChunk(const WorldChunk & chunk) 
    {
        // Load existing
        SaveArchiveData archive;

        if (_openStream(std::ios::in))
        {
            std::stringstream buffer = _readStream();
            _closeStream();
            google::protobuf::util::JsonStringToMessage(buffer.str(), &archive);
        }
        else
        {
            spdlog::error("[save-archive] Failed to open stream for reading JSON");
            return false;
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
        options.always_print_fields_with_no_presence = true;
        options.add_whitespace = true;

        google::protobuf::util::MessageToJsonString(archive, &json, options);
        
        if (!_openStream(std::ios::out))
        {
            spdlog::error("Failed to open stream for writing JSON");
            return false;
        }

        _stream << json;

        _closeStream();

        // append
        if(!replaced)
        {
            // rebuild cache
            _all_chunks.clear();
            for (std::size_t i = 0; i < static_cast<std::size_t>(archive.chunks_size()); ++i)
            {
                const auto& chunk = archive.chunks(static_cast<int>(i));
                _chunk_index[chunk.id()].index = i;
                _all_chunks.emplace(chunk.id());
            }

        }

        return true;
    }

    bool SaveArchive<asset::use_json_t>::updateEntity(const ChunkID& chunk_id,
                                  const ecs::EntityDefinition& entity_def)
    {
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

        // Replace or append entity by name()
        auto* entities = chunk.mutable_entities();
        const std::string& key_name = entity_def.name();

        auto it = std::find_if(entities->begin(), entities->end(),
            [&](const ecs::EntityDefinition& e) {
                return e.name() == key_name;
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

    bool SaveArchive<asset::use_json_t>::removeEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def)
    {
        // open & read
        if (!_openStream(std::ios::in)) {
            spdlog::error("[save-archive] Failed to open stream for reading");
            return false;
        }
        std::stringstream buffer = _readStream();
        _closeStream();


        // parse
        SaveArchiveData archive;
        google::protobuf::util::JsonParseOptions in_opts;
        in_opts.ignore_unknown_fields = true;
        auto st = google::protobuf::util::JsonStringToMessage(buffer.str(), &archive, in_opts);
        if (!st.ok()) {
            spdlog::error("[save-archive] parse error: {}", st.ToString());
            return false;
        }

        // locate chunk
        int chunk_idx = -1;
        for (int i = 0; i < archive.chunks_size(); ++i) {
            if (archive.chunks(i).id() == chunk_id) {
                chunk_idx = i;
                break;
            }
        }
        if (chunk_idx < 0) {
            spdlog::warn("[save-archive] removeEntity: chunk not found");
            return false;
        }

        // remove entity by name
        auto* entities = archive.mutable_chunks(chunk_idx)->mutable_entities();
        const std::string& target = entity_def.name();
        int entity_idx = -1;
        for (int i = 0; i < entities->size(); ++i) 
        {
            if (entities->Get(i).name() == target)
            {
                entity_idx = i;
                break;
            }
        }
        if (entity_idx < 0)
        {
            spdlog::warn("[save-archive] removeEntity: entity '{}' not found in chunk", target);
            return false;
        }
        entities->DeleteSubrange(entity_idx, 1);

        // serialize
        std::string out_json;
        google::protobuf::util::JsonPrintOptions out_opts;
        out_opts.preserve_proto_field_names = true;
        out_opts.always_print_fields_with_no_presence = true; // keep zeros (matches your code path)
        out_opts.add_whitespace = true;

        st = google::protobuf::util::MessageToJsonString(archive, &out_json, out_opts);
        if (!st.ok())
        {
            spdlog::error("[save-archive] serialize error: {}", st.ToString());
            return false;
        }

        // write
        if (!_openStream(std::ios::out | std::ios::trunc))
        {
            spdlog::error("[save-archive] Failed to open stream for writing");
            return false;
        }
        _stream << out_json;
        const bool ok = _stream.good();
        _closeStream();

        if (!ok)
        {
            spdlog::error("[save-archive] write error");
            return false;
        }

        // Note: _chunk_index / _all_chunks unchanged (we only modified entities within a chunk)
        spdlog::debug("[save-archive] entity '{}' removed from chunk ({},{},{})",
                      target, chunk_id.x(), chunk_id.y(), chunk_id.z());
        return true;
    }

    std::optional<WorldChunk> SaveArchive<asset::use_json_t>::readChunk(const ChunkID & id)
    {
        if (!_chunk_index.contains(id))
        {
            return std::nullopt;
        }

        if (!_openStream(std::ios::in)) 
        {
            spdlog::error("Failed to open stream for reading");
            return std::nullopt;
        }

        // we need to read whole message to parse chunks array
        std::stringstream buffer = _readStream();
        _closeStream();

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


    bool SaveArchive<asset::use_json_t>::removeChunk(const ChunkID& id)
    {
        if (!_chunk_index.contains(id))
        {
            return false;
        }
        
        if (!_openStream(std::ios::in)) 
        {
            spdlog::error("[save-archive] Failed to open stream");
            return false;
        }

        // we need to read whole message to parse chunks array
        std::stringstream buffer = _readStream();
        _closeStream();

        SaveArchiveData archive;
        google::protobuf::util::JsonParseOptions in_options;
        in_options.ignore_unknown_fields = true;

        auto status = google::protobuf::util::JsonStringToMessage(buffer.str(), &archive, in_options);
        if (!status.ok()) {
            spdlog::error("[save-archive] parse error: {}", status.ToString());
            return false;
        }

        // Find chunk index
        int remove_idx = -1;
        for (int i = 0; i < archive.chunks_size(); ++i)
        {
            if (archive.chunks(i).id() == id) {
                remove_idx = i;
                break;
            }
        }
        if (remove_idx < 0)
        {
            // index desync: it's in the map but not in file; treat as failure
            spdlog::warn("[save-archive] chunk present in index but not in file");
            return false;
        }

        // Remove and write back
        archive.mutable_chunks()->DeleteSubrange(remove_idx, 1);

        std::string out_json;
        google::protobuf::util::JsonPrintOptions out_options;
        out_options.preserve_proto_field_names = true;
        out_options.always_print_fields_with_no_presence = true;
        out_options.add_whitespace = true;


        status = google::protobuf::util::MessageToJsonString(archive, &out_json, out_options);
        if (!status.ok())
        {
            spdlog::error("[save-archive] serialize error: {}", status.ToString());
            return false;
        }

        if(_openStream(std::ios::out | std::ios::trunc))
        {
            _stream << out_json;
            if (!_stream.good())
            {
                spdlog::error("[save-archive] write error");

                _closeStream();
                return false;
            }
            _closeStream();
            
            // Update caches
            _all_chunks.erase(id);

            // rebuild indexes
            _chunk_index.clear();
            for (int i = 0; i < archive.chunks_size(); ++i) 
            {
                const auto& ch = archive.chunks(i);
                _chunk_index[ch.id()] = ChunkIndexEntry{
                    static_cast<std::size_t>(i),
                    std::streamoff{0}, // not used for JSON
                    std::size_t{0}     // not used for JSON
                };
                // fallback if not present somehow
                _all_chunks.emplace(ch.id());
            }

            spdlog::debug("[save-archive] chunk removed");
            return true;
        }
        else
        {
            spdlog::error("[save-archive] Failed to open stream for writing");
            return false;
        }

        return false;
    }
}
