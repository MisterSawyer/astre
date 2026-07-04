#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <concepts>

#include <asio.hpp>
#include <google/protobuf/message.h>

namespace astre::asset
{
    // An engine-native asset definition: a proto message addressable by name.
    // Satisfied by ShaderDefinition, ScriptDefinition, MeshDefinition, ...
    template<class Def>
    concept AssetDefinition =
        std::derived_from<Def, google::protobuf::Message> &&
        requires(const Def & d) {
            { d.name() } -> std::convertible_to<std::string_view>;
        };

    // Stage 3 (write side): uploads a definition into a runtime system. Name-
    // agnostic — the upload never inspects name(), that is an importer/cache
    // concern (same reasoning DefinitionSource uses on the read side below).
    // Satisfied by EntityLoader for EntityDefinition, ChunkLoader for WorldChunk.
    template<class L, class Def>
    concept LoadSink =
        requires(L & l, const Def & d) {
            { l.load(d) } -> std::same_as<asio::awaitable<bool>>;
        };

    // Named variant, for definitions that also flow through the import/cache
    // stages and are therefore addressable by name().
    template<class L, class Def>
    concept LoaderOf = LoadSink<L, Def> && AssetDefinition<Def>;

    // Stage 2 (read side): where loaders' inputs come from. Satisfied by
    // AssetCache<Def>; WorldStreamer satisfies it for WorldChunk, which is
    // how world streaming bridges into the same pipeline without inheriting
    // from anything.
    // There is no AssetDefinition<Def> requirement here — the read side only
    // yields pointers, it never inspects name(). WorldChunk has no name() and
    // still needs to satisfy this; name-addressability is a cache/importer concern.
    template<class S, class Key, class Def>
    concept DefinitionSource =
        requires(const S & s, Key k) {
            { s.read(k) } -> std::same_as<const Def *>;
        };
}
