#pragma once

#include "loader/shader_loader.hpp"
#include "loader/mesh_loader.hpp"
#include "loader/entity_loader.hpp"
#include "loader/entity_serializer.hpp"
#include "loader/script_loader.hpp"

#include "asset/concepts.hpp"
#include "asset/asset_cache.hpp"

#include "file/world_streamer.hpp"

namespace astre::loader
{
    // Every concept-conformance check for this module's types lives here,
    // in one place, rather than scattered next to each definition.

    static_assert(asset::LoaderOf<ShaderLoader, proto::render::ShaderDefinition>);
    static_assert(asset::LoaderOf<MeshLoader, proto::render::MeshDefinition>);
    static_assert(asset::LoaderOf<ScriptLoader, proto::script::ScriptDefinition>);
    static_assert(asset::LoaderOf<EntityLoader, proto::ecs::EntityDefinition>);

    // WorldStreamer (File module) satisfies DefinitionSource (Asset module)
    // structurally, without either module depending on the other. Loader
    // depends on both already, so the cross-module contract is checked here.
    static_assert(asset::DefinitionSource<file::WorldStreamer, proto::file::ChunkID, proto::file::WorldChunk>);

    // The one fully-constrained pipeline function: import -> cache -> load.
    // Stage 1+2 fan out over the pool; Stage 3 uploads each cached definition
    // into its runtime system. Misuse (wrong importer/loader for Def) fails at
    // this call site with the concept's name in the error.
    template<asset::AssetDefinition Def,
             asset::ImporterOf<Def> Importer,
             asset::LoaderOf<Def> Loader>
    asio::awaitable<bool> loadAssets(std::vector<std::string> names,
                                     Importer import,
                                     asset::AssetCache<Def> & cache,
                                     Loader & loader)
    {
        // Stage 1 + 2: import each source and cache it, fanned out over the pool.
        auto importOne = [&](std::string name) -> asio::awaitable<bool>
        {
            co_await asio::post(cache.executor(), asio::use_awaitable);
            auto def = co_await import(name);
            if(!def)
            {
                spdlog::error("[load-assets] Failed to import {}", name);
                co_return false;
            }
            co_await cache.put(std::move(name), std::move(*def));
            co_return true;
        };

        using op_type = decltype(asio::co_spawn(cache.executor(), importOne(std::string{}), asio::deferred));
        std::vector<op_type> ops;
        ops.reserve(names.size());
        for(const auto & name : names)
            ops.emplace_back(asio::co_spawn(cache.executor(), importOne(name), asio::deferred));

        auto g = asio::experimental::make_parallel_group(std::move(ops));
        const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);
        auto [order, excs, results] = co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);

        for(std::size_t i = 0; i < excs.size(); ++i)
        {
            if(excs[i]) std::rethrow_exception(excs[i]);
            if(!results[i]) co_return false;
        }

        // Stage 3: upload each cached definition into its runtime system.
        for(const auto & name : names)
        {
            const Def * def = cache.read(name);
            if(!def)
            {
                spdlog::error("[load-assets] Missing cached asset {}", name);
                co_return false;
            }
            if(!co_await loader.load(*def))
            {
                spdlog::error("[load-assets] Failed to load {} into runtime", name);
                co_return false;
            }
        }

        co_return true;
    }
}
