#pragma once

#include "script/script.hpp"

#include "proto/Script/script_definition.pb.h"

#include "loader/loader_interface.hpp"

namespace astre::loader
{
    class ScriptLoader : public ILoader
    {
    public:
        ScriptLoader(script::ScriptRuntime & script_runtime)
        : _script_runtime(script_runtime) 
        {}

        virtual ~ScriptLoader() = default;

        asio::awaitable<void> load(const proto::script::ScriptDefinition & script_def)
        {
            std::ostringstream ss;
            for (const auto & code : script_def.code()) ss << code << std::endl;

            _script_runtime.loadScript(script_def.name(), ss.str());
            co_return;
        }


    private:
        script::ScriptRuntime & _script_runtime;

    };
}