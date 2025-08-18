#pragma once

#include <filesystem>

#include "generated/Render/proto/mesh_definition.pb.h"

namespace astre::file
{
    class MeshStreamer
    {
        public:

            std::optional<render::MeshDefinition> readMesh(std::filesystem::path path);
    
        private:

            absl::flat_hash_map<std::string, std::filesystem::path> _avaiable_meshes;
    };
}