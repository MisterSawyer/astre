#include "loader/mesh_loader.hpp"

#include <spdlog/spdlog.h>

#include "math/math.hpp"

namespace astre::loader
{
    asio::awaitable<bool> MeshLoader::load(const proto::render::MeshDefinition & mesh_def) const
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

    asio::awaitable<bool> MeshLoader::load(const std::vector<proto::render::MeshDefinition> & mesh_defs) const
    {
        for(const auto & mesh_def : mesh_defs)
            if(!co_await load(mesh_def)) co_return false;
        co_return true;
    }

    asio::awaitable<bool> MeshLoader::unload(const std::string & name) const
    {
        co_return co_await unload(std::vector<std::string>{name});
    }

    asio::awaitable<bool> MeshLoader::unload(const std::vector<std::string> & names) const
    {
        for(const auto & name : names)
        {
            auto mesh = _renderer.getVertexBuffer(name);
            if(!mesh)
            {
                spdlog::warn("[mesh-loader] Mesh {} was not loaded", name);
                continue;
            }

            co_await _renderer.eraseVertexBuffer(*mesh);
        }

        co_return true;
    }

    asio::awaitable<bool> MeshLoader::loadPrefabs()
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

        if((co_await _renderer.createVertexBuffer("wire_cube_prefab", render::getWireCubePrefab())) == std::nullopt)
        {
            spdlog::error("Failed to create wire_cube_prefab vertex buffer");
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
}
