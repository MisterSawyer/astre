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

        asio::awaitable<void> loadScript(const proto::script::ScriptDefinition & script_def) const
        {

            co_return;
        }


    private:
        script::ScriptRuntime & _script_runtime;

    };
}