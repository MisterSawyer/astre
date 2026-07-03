#pragma once

#include "asset/concepts.hpp"
#include "asset/asset_cache.hpp"
#include "asset/importers.hpp"

namespace astre::asset
{
    // Every concept-conformance check for this module's types lives here,
    // in one place, rather than scattered next to each definition.
    static_assert(ImporterOf<decltype(glslImporter({})), proto::render::ShaderDefinition>);
    static_assert(ImporterOf<decltype(luaImporter({})), proto::script::ScriptDefinition>);
}
