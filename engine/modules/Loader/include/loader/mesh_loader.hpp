#pragma once

#include <string>
#include <vector>

#include <absl/container/flat_hash_set.h>

#include "render/render.hpp"

#include "asset/concepts.hpp"

#include "proto/Render/mesh_definition.pb.h"

namespace astre::loader
{
    class MeshLoader
    {
    public:
        MeshLoader(render::IRenderer & renderer)
        : _renderer(renderer)
        {}

        template<class Keys, class Source>
        asio::awaitable<bool> sync(Keys keys, const Source & source)
        {
            std::vector<std::string> stale;
            for(const auto & key : _loaded_meshes)
                if(!keys.contains(key))
                    stale.push_back(key);

            if(!stale.empty())
                if(!co_await unload(stale)) co_return false;

            for(const auto & key : keys)
            {
                if(_loaded_meshes.contains(key)) continue;

                const auto * mesh = source.read(key);
                if(!mesh) continue;
                if(!co_await load(*mesh)) co_return false;
            }

            co_return true;
        }

        // Stage 3: definition → GPU vertex buffer, keyed by name.
        asio::awaitable<bool> load(const proto::render::MeshDefinition & mesh_def);
        // ponytail: batch = loop over single load; no batched renderer create exists.
        asio::awaitable<bool> load(const std::vector<proto::render::MeshDefinition> & mesh_defs);
        asio::awaitable<bool> unload(const std::vector<std::string> & names);
        asio::awaitable<bool> unload(const std::string & name);
        asio::awaitable<bool> loadPrefabs();

    private:

        render::IRenderer & _renderer;
        absl::flat_hash_set<std::string> _loaded_meshes;

    };
}
