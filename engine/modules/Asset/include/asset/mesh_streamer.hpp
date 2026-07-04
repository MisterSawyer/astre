#pragma once

#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>

#include <asio.hpp>

#include "process/process.hpp"

#include "asset/asset_cache.hpp"
#include "asset/asset_streamer.hpp"

#include "file/data_type.hpp"
#include "file/mesh_file.hpp"

#include "proto/Render/mesh_definition.pb.h"

namespace astre::asset
{
    // Dedicated streamer for meshes; same shape as ShaderStreamer/ScriptStreamer:
    // opens no file at construction, holds only an AssetCache. stream(files, mode)
    // imports each model file into the cache, keyed by its stem (the asset name);
    // keys() and read(name) are the source surface MeshLoader consumes. The mode tag
    // mirrors WorldStreamer::stream(file, use_json) — only file::use_obj is wired
    // today; new formats add a tag and let assimp handle the parse.
    class MeshStreamer
    {
        public:
            explicit MeshStreamer(process::IProcess & process)
            :   _cache(process)
            {}

            template<class Mode>
            asio::awaitable<bool> stream(const std::vector<std::filesystem::path> & files, Mode)
            {
                static_assert(std::is_same_v<Mode, file::use_obj_t>,
                    "MeshStreamer only supports file::use_obj for now");
                return streamAssets(_cache, _source, files,
                    [](const std::filesystem::path & p){ return p.stem().string(); });
            }

            absl::flat_hash_set<std::string> keys() const { return _cache.keys(); }

            const proto::render::MeshDefinition * read(const std::string & name) const
            {
                return _cache.read(name);
            }

        private:
            file::MeshFile _source;
            AssetCache<std::string, proto::render::MeshDefinition> _cache;
    };
}
