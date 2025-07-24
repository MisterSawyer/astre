#include <spdlog/spdlog.h>

#include "asset/asset.hpp"

namespace astre::asset
{
    asio::awaitable<bool> loadVertexBuffersPrefabs(render::IRenderer & renderer)
    {
        bool success = true;

        if((co_await renderer.createVertexBuffer("NDC_quad_prefab", render::getNormalizedDeviceCoordinatesQuadPrefab())) == std::nullopt)
        {
            spdlog::error("Failed to create NDC quad prefab vertex buffer");
            success = false;
        }

        if((co_await renderer.createVertexBuffer("quad_prefab", render::getQuadPrefab())) == std::nullopt)
        {
            spdlog::error("Failed to create quad_prefab vertex buffer");
            success = false;
        }

        if((co_await renderer.createVertexBuffer("cube_prefab", render::getCubePrefab())) == std::nullopt)
        {
            spdlog::error("Failed to create cube_prefab vertex buffer");
            success = false;
        }

        if((co_await renderer.createVertexBuffer("cone_prefab", render::getConePrefab())) == std::nullopt)
        {
            spdlog::error("Failed to create cone_prefab vertex buffer");
            success = false;
        }

        if((co_await renderer.createVertexBuffer("cylinder_prefab", render::getCylinderPrefab())) == std::nullopt)
        {
            spdlog::error("Failed to create cylinder_prefab vertex buffer");
            success = false;
        }

        if((co_await renderer.createVertexBuffer("icosphere1_prefab", render::getIcoSpherePrefab(1))) == std::nullopt)
        {
            spdlog::error("Failed to create icosphere1_prefab vertex buffer");
            success = false;
        }

        if((co_await renderer.createVertexBuffer("icosphere2_prefab", render::getIcoSpherePrefab(2))) == std::nullopt)
        {
            spdlog::error("Failed to create icosphere2_prefab vertex buffer");
            success = false;
        }
        
        if((co_await renderer.createVertexBuffer("icosphere3_prefab", render::getIcoSpherePrefab(3))) == std::nullopt)
        {
            spdlog::error("Failed to create icosphere3_prefab vertex buffer");
            success = false;
        }

        // TODO nie mam pojęcia czemu tego nie widać od środka mimo że : 
        // glFrontFace(GL_CCW); --> traktuj counter-clockwise jako front faces
        // tu ustawiamy że front face to clockwise i normalne do środka :/
        // możliwe że coś z algorytmem windingu 
        if((co_await renderer.createVertexBuffer("dome_prefab", render::getIcoSpherePrefab(3, render::TriangleWinding::Clockwise, true))) == std::nullopt)
        {
            spdlog::error("Failed to create dome_prefab vertex buffer");
            success = false;
        }

        co_return success;
    }


    asio::awaitable<std::optional<std::size_t>> loadShaderFromFile(render::IRenderer & renderer, const std::filesystem::path shader_dir)
    {
        if(!std::filesystem::exists(shader_dir)) {
            spdlog::error("Shader directory does not exist: {}", shader_dir.string());
            co_return std::nullopt;
        }

        std::ifstream shader_file;
        std::string line;
        std::vector<std::string> vs_lines;
        std::vector<std::string> frag_lines;

        // load vertex shader stage

        std::filesystem::path vs_path = shader_dir / "vertex.glsl";
        if (!std::filesystem::exists(vs_path)) {
            spdlog::error("Vertex shader file does not exist: {}", vs_path.string());
            co_return std::nullopt;
        }
        else
        {
            shader_file.open(vs_path);

            if (!shader_file.is_open()) {
                spdlog::error("Failed to open shader file: {}", vs_path.string());
                co_return std::nullopt;
            }

            while (std::getline(shader_file, line)) {
                vs_lines.push_back(line + "\n");
            }
            
            shader_file.close();
        }

        // load fragment shader stage

        std::filesystem::path fs_path = shader_dir / "fragment.glsl";
        if (!std::filesystem::exists(fs_path)) {
            spdlog::warn("Fragment shader file does not exist: {}", fs_path.string());
        }
        else
        {
            shader_file.open(fs_path);
            
            if (!shader_file.is_open()) {
                spdlog::error("Failed to open shader file: {}", fs_path.string());
                co_return std::nullopt;
            }

            while (std::getline(shader_file, line)) {
                frag_lines.push_back(line + "\n");
            }
            
            shader_file.close();
        }

        // create shader in renderer
        
        assert(shader_file.is_open() == false && "Shader file is still open after closing it.");

        const std::string shader_name = shader_dir.stem().string();

        if(vs_lines.empty() == false)
        {
            if(frag_lines.empty())
            {
                spdlog::debug("Loading shader: {} (vertex only)", shader_name);
                co_return co_await renderer.createShader(std::move(shader_name), std::move(vs_lines));
            }
            else
            {
                spdlog::debug("Loading shader: {} (vertex + fragment)", shader_name);
                co_return co_await renderer.createShader(std::move(shader_name), std::move(vs_lines), std::move(frag_lines));
            }
        }
        else
        {
            spdlog::error("Vertex shader file is empty: {}", vs_path.string());
            co_return std::nullopt;
        }
    }

    asio::awaitable<bool> loadShadersFromDir(render::IRenderer & renderer, const std::filesystem::path shader_dir)
    {
        if(!std::filesystem::exists(shader_dir) || !std::filesystem::is_directory(shader_dir)) {
            spdlog::error("Shader directory does not exist: {}", shader_dir.string());
            co_return false;
        }

        spdlog::info("Loading shaders from {}", shader_dir.string());

        for (const std::filesystem::path & shader_path : std::filesystem::directory_iterator(shader_dir)) {
            if (!std::filesystem::is_directory(shader_path)) {
                continue;
            }

            auto shader_id = co_await loadShaderFromFile(renderer, shader_path);
            if (!shader_id) {
                spdlog::warn(std::format("Failed to load shader from {}", shader_path.string()) );
                continue;
            }
        }

        co_return true;
    }
}