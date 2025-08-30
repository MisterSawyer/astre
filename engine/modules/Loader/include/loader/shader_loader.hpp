#pragma once
#include <vector>

#include "render/render.hpp"

#include "proto/Render/shader_definition.pb.h"

#include "loader/loader_interface.hpp"

namespace astre::loader
{
    /*
    * Load shader from shader definition
    */
    class ShaderLoader : public ILoader
    {
    public:
        ShaderLoader(render::IRenderer & renderer)
        : _renderer(renderer) 
        {}

        virtual ~ShaderLoader() = default;

        asio::awaitable<void> loadShader(const proto::render::ShaderDefinition & shader_def) const
        {
            std::vector<std::string> vertex_code; //= shader_def.vertex_code();
            std::vector<std::string> fragment_code; // = shader_def.fragment_code();

            if(fragment_code.empty())
            {
                auto shader = co_await _renderer.createShader(shader_def.name(), std::move(vertex_code));
            }
            else
            {
                auto shader = co_await _renderer.createShader(shader_def.name(), std::move(vertex_code), std::move(fragment_code));
            }

            co_return;
        }


    private:
        render::IRenderer & _renderer;

    };
}