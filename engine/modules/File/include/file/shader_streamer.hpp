#pragma once

#include "generated/Render/proto/shader_definition.pb.h"

namespace astre::file
{
    class ShaderStreamer
    {
        public:
            std::optional<render::ShaderDefinition> readShader(const std::string & name);
    };
}