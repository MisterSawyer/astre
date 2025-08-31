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

        asio::awaitable<bool> load(const proto::render::MeshDefinition & mesh_def) const
        {
            //render::Mesh mesh {.indices = mesh_def.indices(), mesh_def.vertices};
            //auto shader = co_await renderer.createVertexBuffer(shader_def.name, shader_def.vertex, shader_def.fragment);
            co_return;
        }

        asio::awaitable<bool> loadPrefabs()
        {
            bool success = true;

            if((co_await _renderer.createVertexBuffer("NDC_quad_prefab", render::getNormalizedDeviceCoordinatesQuadPrefab())) == std::nullopt)
            {
                spdlog::error("Failed to create NDC quad prefab vertex buffer");
                success = false;
            }

            if((co_await _renderer.createVertexBuffer("quad_prefab", render::getQuadPrefab())) == std::nullopt)
            {
                spdlog::error("Failed to create quad_prefab vertex buffer");
                success = false;
            }

            if((co_await _renderer.createVertexBuffer("cube_prefab", render::getCubePrefab())) == std::nullopt)
            {
                spdlog::error("Failed to create cube_prefab vertex buffer");
                success = false;
            }

            if((co_await _renderer.createVertexBuffer("cone_prefab", render::getConePrefab())) == std::nullopt)
            {
                spdlog::error("Failed to create cone_prefab vertex buffer");
                success = false;
            }

            if((co_await _renderer.createVertexBuffer("cylinder_prefab", render::getCylinderPrefab())) == std::nullopt)
            {
                spdlog::error("Failed to create cylinder_prefab vertex buffer");
                success = false;
            }

            if((co_await _renderer.createVertexBuffer("icosphere1_prefab", render::getIcoSpherePrefab(1))) == std::nullopt)
            {
                spdlog::error("Failed to create icosphere1_prefab vertex buffer");
                success = false;
            }

            if((co_await _renderer.createVertexBuffer("icosphere2_prefab", render::getIcoSpherePrefab(2))) == std::nullopt)
            {
                spdlog::error("Failed to create icosphere2_prefab vertex buffer");
                success = false;
            }

            if((co_await _renderer.createVertexBuffer("icosphere3_prefab", render::getIcoSpherePrefab(3))) == std::nullopt)
            {
                spdlog::error("Failed to create icosphere3_prefab vertex buffer");
                success = false;
            }

            // TODO nie mam pojęcia czemu tego nie widać od środka mimo że : 
            // glFrontFace(GL_CCW); --> traktuj counter-clockwise jako front faces
            // tu ustawiamy że front face to clockwise i normalne do środka :/
            // możliwe że coś z algorytmem windingu 
            if((co_await _renderer.createVertexBuffer("dome_prefab", render::getIcoSpherePrefab(3, render::TriangleWinding::Clockwise, true))) == std::nullopt)
            {
                spdlog::error("Failed to create dome_prefab vertex buffer");
                success = false;
            }

            co_return success;
        }

    private:

        render::IRenderer & _renderer;

    };
}