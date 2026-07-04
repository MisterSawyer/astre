#pragma once

#include <filesystem>
#include <optional>

#include "proto/Render/mesh_definition.pb.h"

namespace astre::file
{
    // Stateless disk reader for meshes, backed by assimp. read(file) imports the
    // model at `file`, naming the definition by `file.stem()`, and flattens every
    // submesh into one vertex/index buffer. OBJ today; assimp handles the rest once
    // more importers are enabled. File owns disk IO; Asset only ever sees the
    // in-memory definition. Stateless and touches an independent file, so it is safe
    // to call concurrently (asset::MeshStreamer fans reads over the pool).
    class MeshFile
    {
        public:
            std::optional<proto::render::MeshDefinition> read(const std::filesystem::path & file) const;
    };
}
