#pragma once

#include "render/render.hpp"

#include "proto/Render/mesh_definition.pb.h"

#include "loader/loader_interface.hpp"

namespace astre::loader
{
    class MeshLoader : public ILoader
    {
    public:
        MeshLoader(render::IRenderer & renderer)
        : _renderer(renderer) 
        {}

        virtual ~MeshLoader() = default;

        asio::awaitable<void> loadMesh(const proto::render::MeshDefinition & mesh_def) const
        {
            //render::Mesh mesh {.indices = mesh_def.indices(), mesh_def.vertices};
            //auto shader = co_await renderer.createVertexBuffer(shader_def.name, shader_def.vertex, shader_def.fragment);
            co_return;
        }


    private:
        render::IRenderer & _renderer;

    };
}