#pragma once

#include <filesystem>

#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>

#include "async/async.hpp"
#include "process/process.hpp"

#include "file/resource_streamer.hpp"

#include "proto/Script/script_definition.pb.h"

namespace astre::file
{
    class ScriptStreamer : public IResourceStreamer<std::string, proto::script::ScriptDefinition>
    {
        public:
                    
            ScriptStreamer(
                process::IProcess::execution_context_type & execution_context,
                std::filesystem::path directory
            )
            :   _async_context(execution_context),
                _directory(directory) 
            {}

            proto::script::ScriptDefinition * read(std::string name) override
            {
                if(_loaded_scripts.contains(name)) return _loaded_scripts.at(name).get();
                
                return nullptr;
            }

            bool write(const proto::script::ScriptDefinition & data) override
            {
                return false;
            }

            bool remove(std::string name) override
            {
                return false;
            }
        
            asio::awaitable<bool> load(std::filesystem::path script_file)
            {
                spdlog::debug("[script-streamer] Loading script from file {}", script_file.string());
                
                std::filesystem::path script_path = _directory / script_file;

                std::ifstream in(script_path);
                if (!in.is_open()) {
                    spdlog::error(std::format("Failed to open lua script: {}", script_path.string()));
                    co_return false;
                }
        
                std::stringstream buffer;
                buffer << in.rdbuf();
                
                proto::script::ScriptDefinition new_def;
                new_def.set_name(script_file.stem().string());
                new_def.add_code(buffer.str());
                
                spdlog::debug("[script-streamer] Script {} loaded", new_def.name());
                co_await _async_context.ensureOnStrand();

                _loaded_scripts.emplace(script_file.stem().string(), std::make_unique<proto::script::ScriptDefinition>(std::move(new_def)));
                
                co_return true;
            }
        
        private:

            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            std::filesystem::path _directory;

            absl::flat_hash_map<std::string, std::unique_ptr<proto::script::ScriptDefinition>> _loaded_scripts;

    };
}