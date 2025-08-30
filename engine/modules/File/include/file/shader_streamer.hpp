#pragma once

#include <filesystem>

#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>

#include "async/async.hpp"
#include "process/process.hpp"

#include "file/resource_streamer.hpp"

#include "proto/Render/shader_definition.pb.h"

namespace astre::file
{
    class ShaderStreamer : public IResourceStreamer<std::string, proto::render::ShaderDefinition>
    {
        public:
            ShaderStreamer(
                process::IProcess::execution_context_type & execution_context,
                std::filesystem::path directory
            )
            :   _async_context(execution_context),
                _directory(directory) 
            {}

            proto::render::ShaderDefinition * read(std::string name) override
            {
                if(_loaded_shaders.contains(name)) return &_loaded_shaders.at(name);
                
                return nullptr;
            }

            bool write(const proto::render::ShaderDefinition & data) override
            {
                return false;
            }

            bool remove(std::string name) override
            {
                return false;
            }
        
        private:
            asio::awaitable<bool> _load(std::string shader_name)
            {
                if(!std::filesystem::exists(_directory))
                {
                    spdlog::error("Shaders base directory does not exist: {}", _directory.string());
                    co_return false;
                }

                std::filesystem::path shader_dir = _directory / shader_name;

                if(!std::filesystem::exists(shader_dir))
                {
                    spdlog::error("Shader source directory does not exist: {}", shader_dir.string());
                    co_return false;
                }

                std::ifstream shader_file;
                std::string line;

                proto::render::ShaderDefinition shader_def;
                shader_def.set_name(shader_name);

                // load vertex shader stage
                std::filesystem::path vs_path = shader_dir / "vertex.glsl";

                if (!std::filesystem::exists(vs_path))
                {
                    spdlog::error("Vertex shader file does not exist: {}", vs_path.string());
                    co_return false;
                }
                else
                {
                    shader_file.open(vs_path);

                    if (!shader_file.is_open())
                    {
                        spdlog::error("Failed to open shader file: {}", vs_path.string());
                        co_return false;
                    }

                    while (std::getline(shader_file, line))
                    {
                        shader_def.add_vertex_code(line + "\n");
                    }
            
                    shader_file.close();
                }

                // load fragment shader stage
                std::filesystem::path fs_path = shader_dir / "fragment.glsl";

                if (!std::filesystem::exists(fs_path))
                {
                    spdlog::warn("Fragment shader file does not exist: {}", fs_path.string());
                }
                else
                {
                    shader_file.open(fs_path);
            
                    if (!shader_file.is_open()) 
                    {
                        spdlog::error("Failed to open shader file: {}", fs_path.string());
                        co_return false;
                    }

                    while (std::getline(shader_file, line))
                    {
                        shader_def.add_fragment_code(line + "\n");
                    }
            
                    shader_file.close();
                }
                
                assert(shader_file.is_open() == false && "Shader file is still open after closing it.");

                // create shader definition
                co_await _async_context.ensureOnStrand();
                _loaded_shaders.emplace(shader_name, std::move(shader_def));

                co_return true;
            }

            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            std::filesystem::path _directory;

            absl::flat_hash_map<std::string, proto::render::ShaderDefinition> _loaded_shaders;

    };
}