#pragma once

#include "math/math.hpp"
#include "render/render.hpp"

#include "asset/concepts.hpp"

#include "proto/Render/mesh_definition.pb.h"

namespace astre::loader
{
    class MeshLoader
    {
    public:
        MeshLoader(render::IRenderer & renderer)
        : _renderer(renderer)
        {}

        // Stage 3: definition → GPU vertex buffer, keyed by name.
        asio::awaitable<bool> load(const proto::render::MeshDefinition & mesh_def) const
        {
            render::Mesh mesh;
            mesh.indices.assign(mesh_def.indices().begin(), mesh_def.indices().end());
            mesh.vertices.reserve(mesh_def.vertices().size());
            for(const auto & v : mesh_def.vertices())
            {
                mesh.vertices.push_back({
                    math::deserialize(v.position()),
                    math::deserialize(v.normal()),
                    math::deserialize(v.uv())
                });
            }

            if((co_await _renderer.createVertexBuffer(mesh_def.name(), mesh)) == std::nullopt)
            {
                spdlog::error("Failed to create vertex buffer for mesh: {}", mesh_def.name());
                co_return false;
            }
            co_return true;
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