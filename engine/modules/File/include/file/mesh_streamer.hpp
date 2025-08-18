#pragma once

#include <filesystem>

#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>

#include "async/async.hpp"
#include "process/process.hpp"

#include "file/resource_streamer.hpp"

#include "generated/Render/proto/mesh_definition.pb.h"

namespace astre::file
{
    class MeshStreamer : public IResourceStreamer<std::string, render::MeshDefinition>
    {
        public:
            MeshStreamer(
                process::IProcess::execution_context_type & execution_context,
                std::filesystem::path directory
            )
            :   _async_context(execution_context),
                _directory(directory) 
            {}

            render::MeshDefinition * read(std::string name) override
            {
                return nullptr;
            }
            
            bool write(const render::MeshDefinition & data) override
            {
                return false;
            }

            bool remove(std::string name) override
            {
                return false;
            }
    
        private:
            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            std::filesystem::path _directory;

            absl::flat_hash_map<std::string, render::MeshDefinition> _loaded_meshes;
    };
}