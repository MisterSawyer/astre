#pragma once

#include <filesystem>
#include <optional>

#include "proto/Render/shader_definition.pb.h"

namespace astre::file
{
    // Stateless disk reader for shaders. read(dir) reads the shader whose GLSL
    // stages live under the directory `dir` (vertex.glsl / fragment.glsl), naming
    // the definition by `dir.stem()`. File owns disk IO; Asset only ever sees the
    // in-memory definition. Stateless and touches independent files, so it is safe
    // to call concurrently (asset::ShaderStreamer fans reads over the pool).
    class ShaderFile
    {
        public:
            std::optional<proto::render::ShaderDefinition> read(const std::filesystem::path & dir) const;
    };
}
