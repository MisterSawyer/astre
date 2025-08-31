#include <cmath>

#include <spdlog/spdlog.h>

#include "file/world_streamer.hpp"

namespace astre::file 
{
    static inline proto::file::ChunkID toChunkID(const math::Vec3 & pos, float size) {
        proto::file::ChunkID id;
        id.set_x(std::floor(pos.x / size));
        id.set_y(std::floor(pos.y / size));
        id.set_z(std::floor(pos.z / size));        
        return id;
    }

    asio::awaitable<void> WorldStreamer::_loadChunk(const proto::file::ChunkID& id) 
    {
        co_await _async_context.ensureOnStrand();
        
        if (_loaded_chunks.contains(id)) co_return;

        spdlog::debug("Loading chunk  ({};{};{})", id.x(), id.y(), id.z());

        auto chunk = _archive->readChunk(id);
        if (!chunk.has_value())
        {
            spdlog::error("Failed to load chunk");
            co_return;
        }
       
        co_await _async_context.ensureOnStrand();
        spdlog::debug("Chunk loaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks[id] = std::make_unique<proto::file::WorldChunk>(std::move(*chunk));
        _to_reload.erase(id);
    }

    asio::awaitable<void> WorldStreamer::_unloadChunk(const proto::file::ChunkID& id) 
    {
        co_await _async_context.ensureOnStrand();

        if (!_loaded_chunks.contains(id)) co_return;

        spdlog::debug("Unloading chunk  ({};{};{})", id.x(), id.y(), id.z());

        co_await _async_context.ensureOnStrand();
        spdlog::debug("Chunk unloaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks.erase(id);
    }

    const absl::flat_hash_set<proto::file::ChunkID> & WorldStreamer::getAllChunks() const
    {
        return _archive->getAllChunks();
    }

    asio::awaitable<void> WorldStreamer::updateLoadPosition(const math::Vec3 & pos)
    {
        co_await _async_context.ensureOnStrand();

        const proto::file::ChunkID center = toChunkID(pos, _chunk_size);

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
                    if(all_available_chunks.contains(id) == false) continue;
                    required.insert(std::move(id));
                }
            }
        }

        // Load missing chunks, or reload if needed
        for (const proto::file::ChunkID& cid : required) {
            if (!_loaded_chunks.contains(cid) || _to_reload.contains(cid))
            {
                co_await _loadChunk(cid);
            }
        }

        // Unload chunks no longer needed
        std::vector<proto::file::ChunkID> toRemove;
        for (const auto& [cid, _] : _loaded_chunks) {
            if (!required.contains(cid)) {
                toRemove.push_back(cid);
            }
        }
        for (const proto::file::ChunkID& cid : toRemove) 
        {
            co_await _unloadChunk(cid);
        }

        co_return;
    }

    proto::file::WorldChunk * WorldStreamer::read(proto::file::ChunkID id)
    {
        if(!_loaded_chunks.contains(id)) return nullptr;
        return _loaded_chunks.at(id).get();
    }

    bool WorldStreamer::write(const proto::file::WorldChunk & chunk)
    {
        if(_loaded_chunks.contains(chunk.id()))
        {
            // if loaded we need also to update it
            _to_reload.emplace(chunk.id());
        }
        return _archive->writeChunk(chunk);
    }

    bool WorldStreamer::remove(proto::file::ChunkID id)
    {
        // schedule unloading of chunk
        // it really does not matter when the chunk is unloaded
        // we hold a reference to it in a memory
        asio::co_spawn(_async_context.executor(), _unloadChunk(id), asio::detached);

        // and we can remove this chunk from the archive
        return _archive->removeChunk(id);
    }

    // bool WorldStreamer::writeEntity(const ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def)
    // {
    //     if(entity_def.id() == 0)
    //     {
    //         spdlog::error("[world] Invalid entity id");
    //         return false;
    //     }
    //     _to_reload.emplace(chunk_id);
    //     return _archive->writeEntity(chunk_id, entity_def);
    // }

    // bool WorldStreamer::removeEntity(const ChunkID & chunk_id, const proto::ecs::EntityDefinition & entity_def)
    // {
    //     _to_reload.emplace(chunk_id);
    //     return _archive->removeEntity(chunk_id, entity_def);
    // }
} // namespace astre::world
