#pragma once

#include "render/render.hpp"

namespace astre::render
{
    class MeshLoader
    {
    public:
        MeshLoader() = default;
        virtual ~MeshLoader() = default;

        asio::awaitable<void> loadShader(const MeshDefinition & mesh_def, Renderer & renderer) const
        {
            //render::Mesh mesh {.indices = mesh_def.indices(), mesh_def.vertices};
            //auto shader = co_await renderer.createVertexBuffer(shader_def.name, shader_def.vertex, shader_def.fragment);
            co_return;
        }


    private:

    };
}