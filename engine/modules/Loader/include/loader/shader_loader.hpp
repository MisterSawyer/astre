#pragma once
#include <vector>

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

        asio::awaitable<bool> load(const proto::render::ShaderDefinition & shader_def) const
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

            co_return true;
        }


    private:
        render::IRenderer & _renderer;

    };
}