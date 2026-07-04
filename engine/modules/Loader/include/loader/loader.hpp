#pragma once

#include "loader/shader_loader.hpp"
#include "loader/mesh_loader.hpp"
#include "loader/entity_loader.hpp"
#include "loader/entity_serializer.hpp"
#include "loader/script_loader.hpp"
#include "loader/chunk_loader.hpp"

#include "asset/concepts.hpp"

namespace astre::loader
{
    // Every concept-conformance check for this module's types lives here,
    // in one place, rather than scattered next to each definition.

    static_assert(asset::LoaderOf<ShaderLoader, proto::render::ShaderDefinition>);
    static_assert(asset::LoaderOf<MeshLoader, proto::render::MeshDefinition>);
    static_assert(asset::LoaderOf<ScriptLoader, proto::script::ScriptDefinition>);
    static_assert(asset::LoaderOf<EntityLoader, proto::ecs::EntityDefinition>);

    // ChunkLoader is the write-side counterpart: a name-agnostic sink for the
    // WorldChunks the streamer yields.
    static_assert(asset::LoadSink<ChunkLoader, proto::file::WorldChunk>);

    // Stage 1: IWorldFile::read is the ChunkID-keyed counterpart to the asset
    // sources' read(name) -> optional<Def>.
    static_assert(requires(const file::IWorldFile & a, proto::file::ChunkID id) {
        { a.read(id) } -> std::same_as<std::optional<proto::file::WorldChunk>>;
    });
}
