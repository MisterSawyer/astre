#pragma once

#include <string>
#include <vector>

#include <absl/container/flat_hash_set.h>

#include "script/script.hpp"

#include "asset/concepts.hpp"

#include "proto/Script/script_definition.pb.h"

namespace astre::loader
{
    class ScriptLoader
    {
    public:
        ScriptLoader(script::ScriptRuntime & script_runtime)
        : _script_runtime(script_runtime)
        {}

        template<class Keys, class Source>
        asio::awaitable<bool> sync(Keys keys, const Source & source)
        {
            std::vector<std::string> stale;
            for(const auto & key : _loaded_scripts)
                if(!keys.contains(key))
                    stale.push_back(key);

            if(!stale.empty())
                if(!co_await unload(stale)) co_return false;

            for(const auto & key : keys)
            {
                if(_loaded_scripts.contains(key)) continue;

                const auto * script = source.read(key);
                if(!script) continue;
                if(!co_await load(*script)) co_return false;
            }

            co_return true;
        }

        asio::awaitable<bool> load(const proto::script::ScriptDefinition & script_def);
        // ponytail: batch = loop over single load; script runtime has no loadScripts.
        asio::awaitable<bool> load(const std::vector<proto::script::ScriptDefinition> & script_defs);
        asio::awaitable<bool> unload(const std::vector<std::string> & names);
        asio::awaitable<bool> unload(const std::string & name);


    private:
        script::ScriptRuntime & _script_runtime;
        absl::flat_hash_set<std::string> _loaded_scripts;

    };
}
