#include "script/script.hpp"
#include "math/math.hpp"

#include <sstream>
#include <spdlog/spdlog.h>

namespace astre::script
{
    ScriptRuntime::ScriptRuntime()
    {
        spdlog::debug("[script] Creating new ScriptRuntime");
        _lua.open_libraries(sol::lib::base, sol::lib::math);
        _astre_table = _lua["astre"].valid()
            ? _lua["astre"]
            : _lua.create_named_table("astre");

        bindMath();
        bindUtility();
        bindComponents();
    }

    ScriptRuntime::~ScriptRuntime()
    {
        spdlog::debug("[script] Destroying ScriptRuntime");
        collectGarbage();
    }

    void ScriptRuntime::bindMath()
    {
        spdlog::debug("[script] Binding math functions");
        
        assert(_astre_table.valid());

        _math_table = _astre_table["math"].valid()
            ? _astre_table["math"]
            : _astre_table.create("math");

        // Types
        _math_table.new_usertype<math::Vec3>("Vec3",
            sol::constructors<math::Vec3(), math::Vec3(float), math::Vec3(float, float, float)>(),
            sol::call_constructor, sol::constructors<math::Vec3(float, float, float)>(),
            "x", &math::Vec3::x,
            "y", &math::Vec3::y,
            "z", &math::Vec3::z,
            sol::meta_function::multiplication, sol::overload(
                [](math::Vec3 a, math::Vec3 b) { return a * b; },
                [](math::Vec3 a, float s) { return a * s; },
                [](float s, math::Vec3 a) { return s * a; }
            ),
            sol::meta_function::addition, [](math::Vec3 a, math::Vec3 b) { return a + b; },
            sol::meta_function::subtraction, [](math::Vec3 a, math::Vec3 b) { return a - b; }
        );

        _math_table.new_usertype<math::Quat>("Quat",
            sol::constructors<math::Quat(), math::Quat(float, float, float, float)>(),
            sol::call_constructor, sol::constructors<math::Quat(const math::Vec3&)>(),
            "w", &math::Quat::w,
            "x", &math::Quat::x,
            "y", &math::Quat::y,
            "z", &math::Quat::z
        );

        // Functions
        _math_table.set_function("radians", [](const math::Vec3& degrees) -> math::Vec3 {
            return math::radians(degrees);
        });

        _math_table.set_function("degrees", [](const math::Vec3& radians) -> math::Vec3 {
            return math::degrees(radians);
        });

        _math_table.set_function("normalize", [](const math::Vec3 & vec3) -> math::Vec3 {
            return math::normalize(vec3);
        });

        _math_table.set_function("length", [](const math::Vec3 & vec3) -> float {
            return math::length(vec3);
        });

        _math_table.set_function("eulerAngles", [](const math::Quat & quat) -> math::Vec3 {
            return math::eulerAngles(quat);
        });

        _astre_table["math"] = _math_table;
    }

    void ScriptRuntime::bindUtility()
    {
        spdlog::debug("[script] Binding utility functions");

        _lua.set_function("print", [](sol::variadic_args args) {
            std::ostringstream oss;
            for (auto arg : args) {
                switch (arg.get_type()) {
                    case sol::type::string: oss << arg.as<std::string>(); break;
                    case sol::type::number: oss << arg.as<double>(); break;
                    case sol::type::boolean: oss << (arg.as<bool>() ? "true" : "false"); break;
                    default: oss << "[unknown]";
                }
                oss << " ";
            }
            spdlog::info("[Lua] {}", oss.str());
        });
    }

    void ScriptRuntime::bindComponents()
    {
        spdlog::debug("[script] Binding components");

        assert(_astre_table.valid());

        // TransformComponent
        std::apply(
            [&](auto&&... bindings) {
                _astre_table.new_usertype<LuaBinding<ecs::TransformComponent>>("TransformComponent", std::forward<decltype(bindings)>(bindings)...);
            },
            LuaBinding<ecs::TransformComponent>::BINDINGS
        );

        //VisualComponent
        std::apply(
            [&](auto&&... bindings) {
                _astre_table.new_usertype<LuaBinding<ecs::VisualComponent>>("VisualComponent", std::forward<decltype(bindings)>(bindings)...);
            },
            LuaBinding<ecs::VisualComponent>::BINDINGS
        );

        //CameraComponent
        std::apply(
            [&](auto&&... bindings) {
                _astre_table.new_usertype<LuaBinding<ecs::CameraComponent>>("CameraComponent", std::forward<decltype(bindings)>(bindings)...);
            },
            LuaBinding<ecs::CameraComponent>::BINDINGS
        );

        //LightComponent
        std::apply(
            [&](auto&&... bindings) {
                _astre_table.new_usertype<LuaBinding<ecs::LightComponent>>("LightComponent", std::forward<decltype(bindings)>(bindings)...);
            },
            LuaBinding<ecs::LightComponent>::BINDINGS
        );

        //InputComponent
        std::apply(
            [&](auto&&... bindings) {
                _astre_table.new_usertype<LuaBinding<ecs::InputComponent>>("InputComponent", std::forward<decltype(bindings)>(bindings)...);
            },
            LuaBinding<ecs::InputComponent>::BINDINGS
        );
    }

    sol::environment & ScriptRuntime::getEnviroment(std::size_t id)
    {
        if(_environments.contains(id))
        {
            return _environments.at(id);
        }

        // create new environment
        sol::environment env(_lua, sol::create, _lua.globals());

        // rebind from global state into sandbox enviroment
        env["print"] = _lua["print"];

        // bind astre namespace
        env["astre"] = _astre_table;

        _environments[id] = env;
        
        return _environments.at(id);
    }

    bool ScriptRuntime::loadScript(const std::string & name, const std::string & code)
    {
        spdlog::debug(std::format("[script] Loading script '{}'", name));

        sol::load_result loaded = _lua.load(code);
        if (!loaded .valid()) 
        {
            sol::error err = loaded;
            spdlog::error(std::format("[script] Failed to compile script '{}': {}", name, err.what()));
            return false;
        }

        _scripts[name] = loaded;

        return true;
    }

    void ScriptRuntime::collectGarbage()
    {
        _lua.collect_garbage();
    }
}
