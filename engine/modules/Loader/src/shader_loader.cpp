#include "loader/shader_loader.hpp"

#include <spdlog/spdlog.h>

namespace astre::loader
{
    asio::awaitable<bool> ShaderLoader::load(const proto::render::ShaderDefinition & shader_def)
    {
        std::vector<std::string> vertex_code(shader_def.vertex_code().begin(), shader_def.vertex_code().end());
        std::vector<std::string> fragment_code(shader_def.fragment_code().begin(), shader_def.fragment_code().end());

        if(fragment_code.empty())
        {
            auto shader = co_await _renderer.createShader(shader_def.name(), std::move(vertex_code));
            if(shader == std::nullopt) co_return false;
        }
        else
        {
            auto shader = co_await _renderer.createShader(shader_def.name(), std::move(vertex_code), std::move(fragment_code));
            if(shader == std::nullopt) co_return false;
        }

        _loaded_shaders.insert(shader_def.name());
        co_return true;
    }

    asio::awaitable<bool> ShaderLoader::load(const std::vector<proto::render::ShaderDefinition> & shader_defs)
    {
        for(const auto & shader_def : shader_defs)
            if(!co_await load(shader_def)) co_return false;
        co_return true;
    }

    asio::awaitable<bool> ShaderLoader::unload(const std::string & name)
    {
        co_return co_await unload(std::vector<std::string>{name});
    }

    asio::awaitable<bool> ShaderLoader::unload(const std::vector<std::string> & names)
    {
        for(const auto & name : names)
        {
            auto shader = _renderer.getShader(name);
            if(!shader)
            {
                spdlog::warn("[shader-loader] Shader {} was not loaded", name);
                _loaded_shaders.erase(name);
                continue;
            }

            co_await _renderer.eraseShader(*shader);
            _loaded_shaders.erase(name);
        }

        co_return true;
    }
}
