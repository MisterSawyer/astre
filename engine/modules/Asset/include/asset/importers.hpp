#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include <asio.hpp>

#include "asset/concepts.hpp"

#include "proto/Render/shader_definition.pb.h"
#include "proto/Script/script_definition.pb.h"

namespace astre::asset
{
    // Stage 1 importers. Pure file-IO halves extracted from the old streamers,
    // logic unchanged. Callers bind the directory via the *Importer factories
    // below; the bound callable is what satisfies ImporterOf. loadAssets posts
    // these onto the thread pool, so the synchronous IO here is off the loop.
    // Definitions in importers.cpp.

    asio::awaitable<std::optional<proto::render::ShaderDefinition>>
    importGLSL(std::filesystem::path directory, std::string name);

    asio::awaitable<std::optional<proto::script::ScriptDefinition>>
    importLua(std::filesystem::path directory, std::string name);

    // Directory-bound importer factories; the returned callable is the ImporterOf.
    // Return an unnamed closure type, so these stay header-defined.
    inline auto glslImporter(std::filesystem::path directory)
    {
        return [directory = std::move(directory)](std::string name)
        { return importGLSL(directory, std::move(name)); };
    }

    inline auto luaImporter(std::filesystem::path directory)
    {
        return [directory = std::move(directory)](std::string name)
        { return importLua(directory, std::move(name)); };
    }
}
