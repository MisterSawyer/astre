#pragma once

#include <absl/container/flat_hash_set.h>
#include <absl/container/node_hash_map.h>

#include "async/async.hpp"
#include "process/process.hpp"

namespace astre::asset
{
    // Stage 2: a keyed in-memory store of definitions. Generic over Key (asset
    // name, ChunkID, ...) and Def; no AssetDefinition requirement — the store
    // never inspects name(), that is the importer's concern (same reasoning as
    // DefinitionSource/LoadSink). node_hash_map keeps read() pointers stable
    // across inserts, which streaming callers rely on. Thread-safe writes via
    // strand.
    template<class Key, class Def>
    class AssetCache
    {
        public:
            explicit AssetCache(process::IProcess & process)
            :   _async_context(process.getExecutionContext())
            {}

            // satisfies DefinitionSource<AssetCache, Key, Def>
            const Def * read(const Key & key) const
            {
                auto it = _cache.find(key);
                return it == _cache.end() ? nullptr : &it->second;
            }

            asio::awaitable<void> put(Key key, Def def)
            {
                co_await _async_context.ensureOnStrand();
                _cache.insert_or_assign(std::move(key), std::move(def));
            }

            void remove(const Key & key)
            {
                _cache.erase(key);
            }

            bool contains(const Key & key) const { return _cache.contains(key); }

            // Snapshot of currently-cached keys — the seam eviction needs to
            // decide what to drop (streaming radius-unload today, a memory-
            // pressure policy later). A copy so callers can remove while iterating.
            absl::flat_hash_set<Key> keys() const
            {
                absl::flat_hash_set<Key> out;
                out.reserve(_cache.size());
                for(const auto & [k, _] : _cache) out.insert(k);
                return out;
            }

            // Serialize onto this cache's strand. Streaming owners run their own
            // state (e.g. reload sets) on it too, so all access to one cache
            // stays single-strand.
            asio::awaitable<void> ensureOnStrand() const { return _async_context.ensureOnStrand(); }

            // underlying thread-pool executor, used to fan out imports
            auto executor() const noexcept { return _async_context.executor(); }

        private:
            async::AsyncContext<process::IProcess::execution_context_type> _async_context;
            absl::node_hash_map<Key, Def> _cache;
    };
}
