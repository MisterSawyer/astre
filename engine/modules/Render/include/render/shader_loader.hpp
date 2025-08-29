#pragma once

#include "render/render.hpp"

namespace astre::render
{
    class ShaderLoader
    {
    public:
        ShaderLoader() = default;
        virtual ~ShaderLoader() = default;

        asio::awaitable<void> loadShader(const ShaderDefinition & shader_def, Renderer & renderer) const
        {
            auto shader = co_await renderer.createShader(shader_def.name, shader_def.vertex, shader_def.fragment);
            co_return;
        }


    private:

    };
}