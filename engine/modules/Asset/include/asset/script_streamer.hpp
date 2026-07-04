#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <asio.hpp>

#include "process/process.hpp"

#include "asset/asset_cache.hpp"
#include "asset/asset_streamer.hpp"

#include "file/script_file.hpp"

#include "proto/Script/script_definition.pb.h"

namespace astre::asset
{
    // Dedicated streamer for Lua scripts; same shape as ShaderStreamer: opens no
    // file at construction, holds only an AssetCache. stream(files) imports each
    // .lua file into the cache, keyed by its stem (the asset name); keys() and
    // read(name) are the source surface ScriptLoader::sync consumes.
    class ScriptStreamer
    {
        public:
            explicit ScriptStreamer(process::IProcess & process)
            :   _cache(process)
            {}

            asio::awaitable<bool> stream(const std::vector<std::filesystem::path> & files)
            {
                return streamAssets(_cache, _source, files,
                    [](const std::filesystem::path & p){ return p.stem().string(); });
            }

            absl::flat_hash_set<std::string> keys() const { return _cache.keys(); }

            const proto::script::ScriptDefinition * read(const std::string & name) const
            {
                return _cache.read(name);
            }

        private:
            file::ScriptFile _source;
            AssetCache<std::string, proto::script::ScriptDefinition> _cache;
    };
}
