#pragma once

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

        asio::awaitable<bool> load(const proto::script::ScriptDefinition & script_def)
        {
            std::ostringstream ss;
            for (const auto & code : script_def.code()) ss << code << std::endl;

            _script_runtime.loadScript(script_def.name(), ss.str());
            co_return true;
        }


    private:
        script::ScriptRuntime & _script_runtime;

    };
}
