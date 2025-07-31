#pragma once

#include <filesystem>
#include <string>

#include <sol/sol.hpp>
#include <absl/container/flat_hash_map.h>

#include "script/component_binding.hpp"

namespace astre::script
{
    class ScriptRuntime
    {
    public:
        ScriptRuntime();
        ~ScriptRuntime();

        void collectGarbage();

        [[nodiscard]] sol::environment & getEnviroment(std::size_t id);

        bool loadScript(const std::string & name, const std::string & code);

        bool scriptLoaded(const std::string & name) const { return _scripts.contains(name); }

        const sol::function & getScript(const std::string & name) const { return _scripts.at(name); }

    private:
        void bindMath();
        void bindUtility();
        void bindComponents();

        sol::state _lua;
        sol::table _astre_table;
        sol::table _math_table;
        absl::flat_hash_map<std::string, sol::function> _scripts;

        absl::flat_hash_map<std::size_t, sol::environment> _environments;
    };

}