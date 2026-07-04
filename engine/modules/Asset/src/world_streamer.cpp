#include <algorithm>
#include <cmath>

#include <spdlog/spdlog.h>

#include "asset/world_streamer.hpp"

namespace astre::asset
{
    static inline proto::file::ChunkID toChunkID(const math::Vec3 & pos, float size) {
        proto::file::ChunkID id;
        id.set_x(std::floor(pos.x / size));
        id.set_y(std::floor(pos.y / size));
        id.set_z(std::floor(pos.z / size));
        return id;
    }

    asio::awaitable<void> WorldStreamer::_unloadChunk(const proto::file::ChunkID& id)
    {
        co_await _cache.ensureOnStrand();

        if (!_cache.contains(id)) co_return;

        spdlog::debug("Unloading chunk  ({};{};{})", id.x(), id.y(), id.z());
        _cache.remove(id);
        spdlog::debug("Chunk unloaded  ({};{};{})", id.x(), id.y(), id.z());
    }

    const absl::flat_hash_set<proto::file::ChunkID> & WorldStreamer::getAllChunks() const
    {
        static const absl::flat_hash_set<proto::file::ChunkID> empty;
        return _archive ? _archive->getAllChunks() : empty;
    }

    proto::file::ChunkID WorldStreamer::chunkIdForPosition(const math::Vec3 & pos) const
    {
        return toChunkID(pos, _chunk_size);
    }

    asio::awaitable<absl::flat_hash_map<proto::file::ChunkID, WorldStreamer::ChunkDebugState>>
    WorldStreamer::snapshotChunkStates() const
    {
        co_await _cache.ensureOnStrand();

        const absl::flat_hash_set<proto::file::ChunkID> resident = _cache.keys();

        // Order-independent precedence, so overwriting from the union below is idempotent.
        const auto classify = [&](const proto::file::ChunkID & id) -> ChunkDebugState
        {
            if (_dirty_chunks.contains(id))     return ChunkDebugState::Dirty;
            if (_to_reload.contains(id))        return ChunkDebugState::ToReload;
            if (resident.contains(id))          return ChunkDebugState::Loaded;
            if (_required_chunks.contains(id))  return ChunkDebugState::Required;
            return ChunkDebugState::Unloaded;
        };

        // per-frame full snapshot; throttle only if chunk counts grow large.
        absl::flat_hash_map<proto::file::ChunkID, ChunkDebugState> out;
        const auto add = [&](const auto & set){ for (const auto & id : set) out[id] = classify(id); };
        add(getAllChunks());
        add(_required_chunks);
        add(resident);
        add(_dirty_chunks);
        add(_to_reload);

        co_return out;
    }

    std::optional<math::Vec3> WorldStreamer::findEntityPosition(ecs::Entity entity) const
    {
        if (!_archive) return std::nullopt;

        for (const auto & chunk_id : _archive->getAllChunks())
        {
            const auto chunk = _archive->read(chunk_id);
            if (!chunk) continue;

            for (const auto & entity_def : chunk->entities())
            {
                if (entity_def.id() == entity && entity_def.has_transform())
                    return math::deserialize(entity_def.transform().position());
            }
        }

        return std::nullopt;
    }

    asio::awaitable<void> WorldStreamer::updateLoadPosition(const math::Vec3 & pos)
    {
        if (!_archive) co_return;   // stream(name, mode) not called yet

        co_await _cache.ensureOnStrand();

        const proto::file::ChunkID center = chunkIdForPosition(pos);

        const auto & all_available_chunks = getAllChunks();

        absl::flat_hash_set<proto::file::ChunkID> required;
        for (int dx = -LOAD_RADIUS; dx <= LOAD_RADIUS; ++dx)
        {
            for (int dy = -LOAD_RADIUS; dy <= LOAD_RADIUS; ++dy)
            {
                for (int dz = -LOAD_RADIUS; dz <= LOAD_RADIUS; ++dz)
                {
                    proto::file::ChunkID id;
                    id.set_x(center.x() + dx);
                    id.set_y(center.y() + dy);
                    id.set_z(center.z() + dz);
                    required.insert(std::move(id));
                }
            }
        }
        _required_chunks = required;

        // Keys needing a (re)load: missing, or resident but marked dirty. streamAssets
        // overwrites via put (insert_or_assign), so it doubles as the reload path.
        std::vector<proto::file::ChunkID> to_load;
        for (const proto::file::ChunkID& cid : required)
        {
            if(!_cache.contains(cid) && !all_available_chunks.contains(cid))
            {
                proto::file::WorldChunk empty;
                empty.mutable_id()->CopyFrom(cid);
                spdlog::debug("Creating empty chunk  ({};{};{})", cid.x(), cid.y(), cid.z());
                co_await _cache.put(cid, std::move(empty));
                spdlog::debug("Empty chunk created  ({};{};{})", cid.x(), cid.y(), cid.z());

                continue;
            }

            if (!_cache.contains(cid) || _to_reload.contains(cid))
                to_load.push_back(cid);
        }

        if (!to_load.empty())
        {
            spdlog::debug("Loading chunks  ({};{};{}),...", to_load.at(0).x(), to_load.at(0).y(), to_load.at(0).z());

            // Same pool fan-out as the other streamers; IWorldFile::read is const
            // and uses a local stream, so the parallel reads are safe.
            if (!co_await streamAssets(_cache, *_archive, to_load))
                spdlog::error("Failed to stream world chunks");

            spdlog::debug("Chunks loaded ({};{};{}),...", to_load.at(0).x(), to_load.at(0).y(), to_load.at(0).z());

            co_await _cache.ensureOnStrand();
            for (const proto::file::ChunkID& cid : to_load) _to_reload.erase(cid);
        }

        co_return;
    }

    const proto::file::WorldChunk * WorldStreamer::read(proto::file::ChunkID id) const
    {
        return _cache.read(id);
    }

    asio::awaitable<bool> WorldStreamer::write(const proto::file::WorldChunk & chunk)
    {
        co_await _cache.ensureOnStrand();

        if(!_archive) co_return false;

        if(_cache.contains(chunk.id()))
        {
            // if loaded we need also to update it
            _to_reload.emplace(chunk.id());
        }
        co_return _archive->writeChunk(chunk);
    }

    asio::awaitable<bool> WorldStreamer::upsertCachedEntity(proto::file::ChunkID id, proto::ecs::EntityDefinition entity_def)
    {
        co_await _cache.ensureOnStrand();

        if(!_archive) co_return false;

        if(entity_def.id() == 0)
        {
            spdlog::error("[world-streamer] Refusing to cache entity with id 0");
            co_return false;
        }

        // Seed the merge target: cache if resident, else the on-disk chunk so we
        // don't clobber its other entities when re-homing into a non-resident chunk.
        proto::file::WorldChunk chunk;
        if(const auto * existing = _cache.read(id))
            chunk.CopyFrom(*existing);
        else if(auto on_disk = _archive->read(id))
            chunk = std::move(*on_disk);
        else
            chunk.mutable_id()->CopyFrom(id);

        auto * entities = chunk.mutable_entities();
        auto it = std::find_if(entities->begin(), entities->end(),
            [&](const proto::ecs::EntityDefinition & existing)
            {
                return existing.id() == entity_def.id();
            });

        if(it != entities->end())
            it->CopyFrom(entity_def);
        else
            entities->Add()->CopyFrom(entity_def);

        // Deferred write: cache holds the newest copy, disk catches up on unload
        // via _persistChunkIfDirty (or persistAll at shutdown).
        _dirty_chunks.insert(id);
        co_await _cache.put(std::move(id), std::move(chunk));
        co_return true;
    }

    asio::awaitable<bool> WorldStreamer::removeCachedEntity(proto::file::ChunkID id, ecs::Entity entity)
    {
        co_await _cache.ensureOnStrand();

        if(!_archive) co_return false;

        const auto * existing = _cache.read(id);
        if(!existing)
        {
            spdlog::error("[world-streamer] Missing cached chunk ({}, {}, {}) while removing entity {}",
                id.x(), id.y(), id.z(), entity);
            co_return false;
        }

        proto::file::WorldChunk chunk;
        chunk.CopyFrom(*existing);

        auto * entities = chunk.mutable_entities();
        for(int i = 0; i < entities->size(); ++i)
        {
            if(entities->Get(i).id() == entity)
            {
                entities->DeleteSubrange(i, 1);
                _dirty_chunks.insert(id);
                co_await _cache.put(std::move(id), std::move(chunk));
                co_return true;
            }
        }

        spdlog::warn("[world-streamer] Entity {} not found in cached chunk ({}, {}, {})",
            entity, id.x(), id.y(), id.z());
        co_return true;
    }

    asio::awaitable<bool> WorldStreamer::_persistChunkIfDirty(const proto::file::ChunkID& id)
    {
        co_await _cache.ensureOnStrand();

        if(!_archive || !_dirty_chunks.contains(id)) co_return true;

        const auto * chunk = _cache.read(id);
        if(!chunk)
        {
            _dirty_chunks.erase(id);
            co_return true;
        }

        if(chunk->entities_size() == 0 && !_archive->getAllChunks().contains(id))
        {
            _dirty_chunks.erase(id);
            co_return true;
        }

        if(!_archive->writeChunk(*chunk))
        {
            spdlog::error("[world-streamer] Failed to persist dirty chunk ({}, {}, {})",
                id.x(), id.y(), id.z());
            co_return false;
        }

        spdlog::debug("[world-streamer] Persisted dirty chunk ({}, {}, {})",
            id.x(), id.y(), id.z());

        _dirty_chunks.erase(id);
        co_return true;
    }

    asio::awaitable<bool> WorldStreamer::persistAll()
    {
        co_await _cache.ensureOnStrand();

        spdlog::debug("[world-streamer] Persisting {} dirty chunks", _dirty_chunks.size());

        // _persistChunkIfDirty erases from _dirty_chunks, so snapshot the ids first.
        const std::vector<proto::file::ChunkID> dirty(_dirty_chunks.begin(), _dirty_chunks.end());
        for(const auto & id : dirty)
            if(!co_await _persistChunkIfDirty(id)) co_return false;

        co_return true;
    }

    asio::awaitable<bool> WorldStreamer::unloadCachedChunks(const std::vector<proto::file::ChunkID> & ids)
    {
        for(const auto & id : ids)
        {
            if(!co_await _persistChunkIfDirty(id)) co_return false;
            co_await _unloadChunk(id);
        }

        co_return true;
    }

    asio::awaitable<bool> WorldStreamer::remove(proto::file::ChunkID id)
    {
        co_await _cache.ensureOnStrand();

        if(!_archive) co_return false;

        // drop it from memory, then remove it from the archive
        co_await _unloadChunk(id);
        _to_reload.erase(id);
        _dirty_chunks.erase(id);

        co_return _archive->removeChunk(id);
    }
}
