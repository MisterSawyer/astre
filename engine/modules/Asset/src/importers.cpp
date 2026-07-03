#include "asset/importers.hpp"

#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>

namespace astre::asset
{
    asio::awaitable<std::optional<proto::render::ShaderDefinition>>
    importGLSL(std::filesystem::path directory, std::string name)
    {
        spdlog::debug("[import-glsl] Importing shader: {}", name);

        if(!std::filesystem::exists(directory))
        {
            spdlog::error("Shaders base directory does not exist: {}", directory.string());
            co_return std::nullopt;
        }

        const std::filesystem::path shader_dir = directory / name;
        if(!std::filesystem::exists(shader_dir))
        {
            spdlog::error("Shader source directory does not exist: {}", shader_dir.string());
            co_return std::nullopt;
        }

        proto::render::ShaderDefinition shader_def;
        shader_def.set_name(name);

        std::ifstream shader_file;
        std::string line;

        // vertex stage (required)
        const std::filesystem::path vs_path = shader_dir / "vertex.glsl";
        if(!std::filesystem::exists(vs_path))
        {
            spdlog::error("Vertex shader file does not exist: {}", vs_path.string());
            co_return std::nullopt;
        }
        shader_file.open(vs_path);
        if(!shader_file.is_open())
        {
            spdlog::error("Failed to open shader file: {}", vs_path.string());
            co_return std::nullopt;
        }
        while(std::getline(shader_file, line)) shader_def.add_vertex_code(line + "\n");
        shader_file.close();

        // fragment stage (optional)
        const std::filesystem::path fs_path = shader_dir / "fragment.glsl";
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
                co_return std::nullopt;
            }
            while(std::getline(shader_file, line)) shader_def.add_fragment_code(line + "\n");
            shader_file.close();
        }

        co_return shader_def;
    }

    asio::awaitable<std::optional<proto::script::ScriptDefinition>>
    importLua(std::filesystem::path directory, std::string name)
    {
        const std::filesystem::path script_path = directory / (name + ".lua");
        spdlog::debug("[import-lua] Importing script from file {}", script_path.string());

        std::ifstream in(script_path);
        if(!in.is_open())
        {
            spdlog::error("Failed to open lua script: {}", script_path.string());
            co_return std::nullopt;
        }

        std::stringstream buffer;
        buffer << in.rdbuf();

        proto::script::ScriptDefinition def;
        def.set_name(name);
        def.add_code(buffer.str());

        co_return def;
    }
}
