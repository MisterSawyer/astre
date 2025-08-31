#pragma once

#include <filesystem>
#include <vector>

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

            proto::script::ScriptDefinition * read(std::string name) const override
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
        
            asio::awaitable<bool> load(std::vector<std::filesystem::path> script_files)
            {
                using op_type = decltype(asio::co_spawn(_async_context.executor(), _load(""), asio::deferred));
                std::vector<op_type> ops;
                ops.reserve(script_files.size());

                for(const auto & script_file : script_files)
                {
                    ops.emplace_back(asio::co_spawn(_async_context.executor(), _load(script_file), asio::deferred));
                }

                auto g = asio::experimental::make_parallel_group(std::move(ops));
                
                const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);

                co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);
            }
            private:

            asio::awaitable<bool> _load(std::filesystem::path script_file)
            {
                // we want to post those jobs asynchronously so we need to create suspension point
                // for the thread_pool to be able to pick up other jobs from the queue
                co_await asio::post(_async_context.executor(), asio::use_awaitable);

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