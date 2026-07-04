#include "file/shader_file.hpp"

#include <fstream>

#include <spdlog/spdlog.h>

namespace astre::file
{
    std::optional<proto::render::ShaderDefinition> ShaderFile::read(const std::filesystem::path & dir) const
    {
        const std::string name = dir.stem().string();
        spdlog::debug("[shader-file] Reading shader: {}", name);

        if(!std::filesystem::exists(dir))
        {
            spdlog::error("Shader source directory does not exist: {}", dir.string());
            return std::nullopt;
        }

        proto::render::ShaderDefinition shader_def;
        shader_def.set_name(name);

        std::ifstream shader_file;
        std::string line;

        // vertex stage (required)
        const std::filesystem::path vs_path = dir / "vertex.glsl";
        if(!std::filesystem::exists(vs_path))
        {
            spdlog::error("Vertex shader file does not exist: {}", vs_path.string());
            return std::nullopt;
        }
        shader_file.open(vs_path);
        if(!shader_file.is_open())
        {
            spdlog::error("Failed to open shader file: {}", vs_path.string());
            return std::nullopt;
        }
        while(std::getline(shader_file, line)) shader_def.add_vertex_code(line + "\n");
        shader_file.close();

        // fragment stage (optional)
        const std::filesystem::path fs_path = dir / "fragment.glsl";
        if(!std::filesystem::exists(fs_path))
        {
            spdlog::warn("Fragment shader file does not exist: {}", fs_path.string());
        }
        else
        {
            shader_file.open(fs_path);
            if(!shader_file.is_open())
            {
                spdlog::error("Failed to open shader file: {}", fs_path.string());
                return std::nullopt;
            }
            while(std::getline(shader_file, line)) shader_def.add_fragment_code(line + "\n");
            shader_file.close();
        }

        return shader_def;
    }
}
