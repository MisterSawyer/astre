#pragma once

#include <filesystem>
#include <optional>

#include "proto/Script/script_definition.pb.h"

namespace astre::file
{
    // Stateless disk reader for Lua scripts. read(file) reads the .lua file at
    // `file`, naming the definition by `file.stem()`. File owns disk IO; Asset
    // only ever sees the in-memory definition. Stateless and touches an
    // independent file, so it is safe to call concurrently (asset::ScriptStreamer
    // fans reads over the pool).
    class ScriptFile
    {
        public:
            std::optional<proto::script::ScriptDefinition> read(const std::filesystem::path & file) const;
    };
}
