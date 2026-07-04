#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <asio.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <spdlog/spdlog.h>

#include "asset/asset_cache.hpp"

namespace astre::asset
{
    // Stage 1+2: import each source arg into the cache, fanned out over the pool.
    // The read arg and the cache key can differ:
    // ShaderStreamer/ScriptStreamer read a full path and key by its stem
    // (keyFn = path.stem()), WorldStreamer reads a ChunkID and keys by it
    // (keyFn = identity, via the overload below). A Source is any disk reader with
    // read(arg) const -> optional<Def>. Each read runs on a pool thread, so
    // Source::read must be safe to call concurrently. This is the shared body
    // every streamer forwards its stream/load step to.
    template<class Key, class Def, class Arg, class Source, class KeyFn>
        requires requires(const Source & s, const Arg & a) {
            { s.read(a) } -> std::same_as<std::optional<Def>>;
        }
    asio::awaitable<bool> streamAssets(AssetCache<Key, Def> & cache,
                                       const Source & source,
                                       const std::vector<Arg> & args,
                                       KeyFn keyFn)
    {
        auto importOne = [&](Arg arg) -> asio::awaitable<bool>
        {
            co_await asio::post(cache.executor(), asio::use_awaitable);
            Key key = keyFn(arg);
            auto def = source.read(arg);
            if(!def)
            {
                if constexpr (std::convertible_to<const Key &, std::string_view>)
                    spdlog::error("[stream-assets] Failed to import {}", std::string_view(key));
                else
                    spdlog::error("[stream-assets] Failed to import a source key");
                co_return false;
            }
            co_await cache.put(std::move(key), std::move(*def));
            co_return true;
        };

        using op_type = decltype(asio::co_spawn(cache.executor(), importOne(Arg{}), asio::deferred));
        std::vector<op_type> ops;
        ops.reserve(args.size());
        for(const auto & arg : args)
            ops.emplace_back(asio::co_spawn(cache.executor(), importOne(arg), asio::deferred));

        auto g = asio::experimental::make_parallel_group(std::move(ops));
        const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);
        auto [order, excs, results] = co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);

        for(std::size_t i = 0; i < excs.size(); ++i)
        {
            if(excs[i]) std::rethrow_exception(excs[i]);
            if(!results[i]) co_return false;
        }
        co_return true;
    }

    // Identity form: the read arg is itself the cache key (WorldStreamer's ChunkID).
    template<class Key, class Def, class Source>
        requires requires(const Source & s, const Key & k) {
            { s.read(k) } -> std::same_as<std::optional<Def>>;
        }
    asio::awaitable<bool> streamAssets(AssetCache<Key, Def> & cache,
                                       const Source & source,
                                       const std::vector<Key> & keys)
    {
        return streamAssets<Key, Def, Key, Source>(cache, source, keys, std::identity{});
    }
}
