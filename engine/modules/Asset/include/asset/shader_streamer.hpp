#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <asio.hpp>

#include "process/process.hpp"

#include "asset/asset_cache.hpp"
#include "asset/asset_streamer.hpp"

#include "file/shader_file.hpp"

#include "proto/Render/shader_definition.pb.h"

namespace astre::asset
{
    // Dedicated streamer for shaders, mirroring WorldStreamer: opens no file at
    // construction, holds only an AssetCache. stream(files) imports each shader
    // directory into the cache, keyed by its stem (the asset name); keys() and
    // read(name) are the source surface ShaderLoader::sync consumes.
    class ShaderStreamer
    {
        public:
            explicit ShaderStreamer(process::IProcess & process)
            :   _cache(process)
            {}

            asio::awaitable<bool> stream(const std::vector<std::filesystem::path> & files)
            {
                return streamAssets(_cache, _source, files,
                    [](const std::filesystem::path & p){ return p.stem().string(); });
            }

            absl::flat_hash_set<std::string> keys() const { return _cache.keys(); }

            const proto::render::ShaderDefinition * read(const std::string & name) const
            {
                return _cache.read(name);
            }

        private:
            file::ShaderFile _source;
            AssetCache<std::string, proto::render::ShaderDefinition> _cache;
    };
}
