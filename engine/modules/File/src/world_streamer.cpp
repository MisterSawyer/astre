#include <cmath>

#include <spdlog/spdlog.h>

#include "file/world_streamer.hpp"

namespace astre::file 
{
    static inline ChunkID toChunkID(const math::Vec3 & pos, float size) {
        ChunkID id;
        id.set_x(std::floor(pos.x / size));
        id.set_y(std::floor(pos.y / size));
        id.set_z(std::floor(pos.z / size));        
        return id;
    }

    asio::awaitable<void> WorldStreamer::_loadChunk(const ChunkID& id) 
    {
        co_await _unloadChunk(id);
        co_await _async_context.ensureOnStrand();
        
        const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);

        spdlog::debug("Loading chunk  ({};{};{})", id.x(), id.y(), id.z());

        auto chunk = _archive->readChunk(id);
        if (!chunk.has_value())
        {
            spdlog::error("Failed to load chunk");
            co_return;
        }
       
        co_await _async_context.ensureOnStrand();
        spdlog::debug("Chunk loaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks[id] = std::move(*chunk);
        _to_reload.erase(id);
    }

    asio::awaitable<void> WorldStreamer::_unloadChunk(const ChunkID& id) 
    {
        co_await _async_context.ensureOnStrand();

        if (!_loaded_chunks.contains(id)) co_return;

        spdlog::debug("Unloading chunk  ({};{};{})", id.x(), id.y(), id.z());

        co_await _async_context.ensureOnStrand();
        spdlog::debug("Chunk unloaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks.erase(id);
    }

    asio::awaitable<void> WorldStreamer::saveAll()
    {
        co_await _async_context.ensureOnStrand();
        for (const auto& [_, chunk] : _loaded_chunks) {
            _archive->writeChunk(chunk);
        }
    }

    const absl::flat_hash_set<ChunkID> & WorldStreamer::getAllChunks() const
    {
        return _archive->getAllChunks();
    }

    asio::awaitable<void> WorldStreamer::updateLoadPosition(const math::Vec3 & pos)
    {
        co_await _async_context.ensureOnStrand();

        const ChunkID center = toChunkID(pos, _chunk_size);

        const auto & all_available_chunks = getAllChunks();

        absl::flat_hash_set<ChunkID> required;
        for (int dx = -LOAD_RADIUS; dx <= LOAD_RADIUS; ++dx)
        {
            for (int dy = -LOAD_RADIUS; dy <= LOAD_RADIUS; ++dy) 
            {
                for (int dz = -LOAD_RADIUS; dz <= LOAD_RADIUS; ++dz)
                { 
                    ChunkID id;
                    id.set_x(center.x() + dx);
                    id.set_y(center.y() + dy);
                    id.set_z(center.z() + dz);
                    if(all_available_chunks.contains(id) == false) continue;
                    required.insert(std::move(id));
                }
            }
        }

        // Load missing chunks, or reload if needed
        for (const ChunkID& cid : required) {
            if (!_loaded_chunks.contains(cid) || _to_reload.contains(cid))
            {
                co_await _loadChunk(cid);
            }
        }

        // Unload chunks no longer needed
        std::vector<ChunkID> toRemove;
        for (const auto& [cid, _] : _loaded_chunks) {
            if (!required.contains(cid)) {
                toRemove.push_back(cid);
            }
        }
        for (const ChunkID& cid : toRemove) 
        {
            co_await _unloadChunk(cid);
        }

        co_return;
    }

    std::optional<WorldChunk> WorldStreamer::readChunk(const ChunkID& id)
    {
        return _archive->readChunk(id);
    }

    bool WorldStreamer::writeChunk(const WorldChunk & chunk)
    {
        return _archive->writeChunk(chunk);
    }

    bool WorldStreamer::removeChunk(const ChunkID& id)
    {
        // schedule unloading of chunk
        // it really does not matter when the chunk is unloaded
        // we hold a reference to it in a memory
        asio::co_spawn(_async_context.executor(), _unloadChunk(id), asio::detached);

        // and we can remove this chunk from the archive
        return _archive->removeChunk(id);
    }

    bool WorldStreamer::writeEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def)
    {
        if(entity_def.id() == 0)
        {
            spdlog::error("[world] Invalid entity id");
            return false;
        }
        _to_reload.emplace(chunk_id);
        return _archive->writeEntity(chunk_id, entity_def);
    }

    bool WorldStreamer::removeEntity(const ChunkID & chunk_id, const ecs::EntityDefinition & entity_def)
    {
        _to_reload.emplace(chunk_id);
        return _archive->removeEntity(chunk_id, entity_def);
    }
} // namespace astre::world
