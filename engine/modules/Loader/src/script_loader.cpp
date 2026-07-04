#include "loader/script_loader.hpp"

#include <sstream>

#include <spdlog/spdlog.h>

namespace astre::loader
{
    asio::awaitable<bool> ScriptLoader::load(const proto::script::ScriptDefinition & script_def)
    {
        std::ostringstream ss;
        for (const auto & code : script_def.code()) ss << code << std::endl;

        if(!_script_runtime.loadScript(script_def.name(), ss.str()))
        {
            spdlog::error("[script-loader] Failed to load script {}", script_def.name());
            co_return false;
        }

        _loaded_scripts.insert(script_def.name());
        co_return true;
    }

    asio::awaitable<bool> ScriptLoader::load(const std::vector<proto::script::ScriptDefinition> & script_defs)
    {
        for(const auto & script_def : script_defs)
            if(!co_await load(script_def)) co_return false;
        co_return true;
    }

    asio::awaitable<bool> ScriptLoader::unload(const std::string & name)
    {
        co_return co_await unload(std::vector<std::string>{name});
    }

    asio::awaitable<bool> ScriptLoader::unload(const std::vector<std::string> & names)
    {
        _script_runtime.unloadScripts(names);
        for(const auto & name : names) _loaded_scripts.erase(name);
        co_return true;
    }
}
