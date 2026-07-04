#pragma once
#include <string>
#include <vector>

#include <absl/container/flat_hash_set.h>

#include "render/render.hpp"

#include "asset/concepts.hpp"

#include "proto/Render/shader_definition.pb.h"

namespace astre::loader
{
    /*
    * Load shader from shader definition
    */
    class ShaderLoader
    {
    public:
        ShaderLoader(render::IRenderer & renderer)
        : _renderer(renderer)
        {}

        template<class Keys, class Source>
        asio::awaitable<bool> sync(Keys keys, const Source & source)
        {
            std::vector<std::string> stale;
            for(const auto & key : _loaded_shaders)
                if(!keys.contains(key))
                    stale.push_back(key);

            if(!stale.empty())
                if(!co_await unload(stale)) co_return false;

            for(const auto & key : keys)
            {
                if(_loaded_shaders.contains(key)) continue;

                const auto * shader = source.read(key);
                if(!shader) continue;
                if(!co_await load(*shader)) co_return false;
            }

            co_return true;
        }

        asio::awaitable<bool> load(const proto::render::ShaderDefinition & shader_def);
        // ponytail: batch = loop over single load; no batched renderer create exists.
        asio::awaitable<bool> load(const std::vector<proto::render::ShaderDefinition> & shader_defs);
        asio::awaitable<bool> unload(const std::vector<std::string> & names);
        asio::awaitable<bool> unload(const std::string & name);

    private:
        render::IRenderer & _renderer;
        absl::flat_hash_set<std::string> _loaded_shaders;

    };
}
