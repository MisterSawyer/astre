#pragma once

#include "asset/concepts.hpp"
#include "asset/asset_cache.hpp"
#include "asset/shader_streamer.hpp"
#include "asset/script_streamer.hpp"
#include "asset/mesh_streamer.hpp"
#include "asset/world_streamer.hpp"

namespace astre::asset
{
    // Every concept-conformance check for this module's types lives here,
    // in one place, rather than scattered next to each definition.

    // Each streamer reads through its AssetCache, so loaders can sync runtime
    // state from the streamer's current keys.
    static_assert(DefinitionSource<WorldStreamer, proto::file::ChunkID, proto::file::WorldChunk>);
    static_assert(DefinitionSource<ShaderStreamer, std::string, proto::render::ShaderDefinition>);
    static_assert(DefinitionSource<ScriptStreamer, std::string, proto::script::ScriptDefinition>);
    static_assert(DefinitionSource<MeshStreamer, std::string, proto::render::MeshDefinition>);
}
