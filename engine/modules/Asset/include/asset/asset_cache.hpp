#pragma once

#include <string>
#include <string_view>

#include <absl/container/flat_hash_map.h>

#include "async/async.hpp"
#include "process/process.hpp"

#include "asset/concepts.hpp"

namespace astre::asset
{
    // Stage 2: one concept-constrained template replaces every hand-written
    // streamer. AssetCache<ShaderDefinition>, AssetCache<ScriptDefinition>, ...
    // Instantiating with a non-definition type is a compile error naming
    // AssetDefinition. Thread-safe writes via strand, same discipline the old
    // streamers used.
    template<asset::AssetDefinition Def>
    class AssetCache
    {
        public:
            explicit AssetCache(process::IProcess::execution_context_type & execution_context)
            :   _async_context(execution_context)
            {}

            // satisfies DefinitionSource<AssetCache, std::string_view, Def>
            const Def * read(std::string_view name) const
            {
                auto it = _cache.find(name);
                return it == _cache.end() ? nullptr : &it->second;
            }

            asio::awaitable<void> put(std::string name, Def def)
            {
                co_await _async_context.ensureOnStrand();
                _cache.insert_or_assign(std::move(name), std::move(def));
            }

            void remove(std::string_view name)
            {
                _cache.erase(name);
            }

            // underlying thread-pool executor, used to fan out imports
            auto executor() const noexcept { return _async_context.executor(); }

        private:
            async::AsyncContext<process::IProcess::execution_context_type> _async_context;
            absl::flat_hash_map<std::string, Def> _cache;
    };
}
