#pragma once

#include <filesystem>
#include <vector>

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

            proto::render::ShaderDefinition * read(std::string name) const override
            {
                if(_loaded_shaders.contains(name)) return _loaded_shaders.at(name).get();
                
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
        
            asio::awaitable<bool> load(std::vector<std::string> shader_names)
            {
                using op_type = decltype(asio::co_spawn(_async_context.executor(), _load(""), asio::deferred));
                std::vector<op_type> ops;
                ops.reserve(shader_names.size());

                for(const auto & shader_name : shader_names)
                {
                    ops.emplace_back(asio::co_spawn(_async_context.executor(), _load(shader_name), asio::deferred));
                }

                auto g = asio::experimental::make_parallel_group(std::move(ops));
                
                const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);

                co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);
            }

            private:
            asio::awaitable<bool> _load(std::string shader_name)
            {   
                // we want to post those jobs asynchronously so we need to create suspension point
                // for the thread_pool to be able to pick up other jobs from the queue
                co_await asio::post(_async_context.executor(), asio::use_awaitable);   

                spdlog::debug("[shader-streamer] Loading shader: {}", shader_name);

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
                co_await _async_context.ensureOnStrand(); // suspends
                _loaded_shaders.emplace(shader_name, std::make_unique<proto::render::ShaderDefinition>(std::move(shader_def)));

                co_return true;
            }
        
        private:

            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            std::filesystem::path _directory;

            absl::flat_hash_map<std::string, std::unique_ptr<proto::render::ShaderDefinition>> _loaded_shaders;

    };
}