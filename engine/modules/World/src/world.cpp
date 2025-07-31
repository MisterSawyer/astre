#include <cmath>

#include <spdlog/spdlog.h>

#include "world/world.hpp"

namespace astre::world {

    static constexpr int LOAD_RADIUS = 0;

    WorldStreamer::WorldStreamer(
        process::IProcess::execution_context_type & execution_context,
        std::filesystem::path path,
        ecs::Registry& registry,
        float chunk_size,
        unsigned int max_loaded_chunks
    )
        :   _async_context(execution_context),
            _archive(std::move(path), asset::use_json),
            _registry(registry),
            _loader(registry),
            _serializer(registry),
            _chunk_size(chunk_size),
            _max_loaded_chunks(max_loaded_chunks)
    {}

    static inline ChunkID toChunkID(const math::Vec3 & pos, float size) {
        ChunkID id;
        id.set_x(std::floor(pos.x / size));
        id.set_y(std::floor(pos.y / size));
        id.set_z(std::floor(pos.z / size));        
        return id;
    }

    asio::awaitable<void> WorldStreamer::addEntitiesToChunk(ChunkID chunk_id, std::vector<ecs::Entity> entities)
    {
        for(const auto & e : entities)
        {
            co_await _async_context.ensureOnStrand();

            _chunk_entities[chunk_id].insert(e);
            _loaded_chunks[chunk_id].add_entities()->CopyFrom(co_await _serializer.serializeEntity(e));
        }
    }

    asio::awaitable<void> WorldStreamer::updateLoadPosition(const math::Vec3 & pos)
    {
        asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

        if(cs.cancelled() != asio::cancellation_type::none)
        {
            spdlog::debug("[world-streamer] updateLoadPosition cancelled");
            co_return;
        }
        co_await _async_context.ensureOnStrand();

        const ChunkID center = toChunkID(pos, _chunk_size);

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
                    required.insert(std::move(id));
                }
            }
        }

        // Load missing chunks
        for (const ChunkID& cid : required) {
            if (!_loaded_chunks.contains(cid))
            {
                if(cs.cancelled() != asio::cancellation_type::none)
                {
                    spdlog::debug("[world-streamer] updateLoadPosition cancelled");
                    co_return;
                }
                co_await loadChunk(cid);
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
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("[world-streamer] updateLoadPosition cancelled");
                co_return;
            }
            co_await unloadChunk(cid);
        }

        co_return;
    }

    asio::awaitable<void> WorldStreamer::loadChunk(const ChunkID& id) 
    {
        asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

        if(cs.cancelled() != asio::cancellation_type::none)
        {
            spdlog::debug("[world-streamer] loadChunk cancelled");
            co_return;
        }
        co_await _async_context.ensureOnStrand();

        if(_loaded_chunks.contains(id)) co_return;

        spdlog::debug("Loading chunk  ({};{};{})", id.x(), id.y(), id.z());

        auto chunk = _archive.readChunk(id, asset::use_json);
        if (!chunk.has_value())
        {
            spdlog::error("Failed to load chunk");
            co_return;
        }

        // load chunk entites
        for (const auto& entity_def : (*chunk).entities()) 
        {
            if(cs.cancelled() != asio::cancellation_type::none)
            {
                spdlog::debug("[world-streamer] loadChunk cancelled");
                co_return;
            }
            std::optional<ecs::Entity> entity = co_await _registry.createEntity(entity_def.name());
            co_await _async_context.ensureOnStrand();

            if (entity.has_value()) {
                _chunk_entities[id].insert(*entity);
                co_await _loader.loadEntity(entity_def, *entity);
            }
        }
        
        co_await _async_context.ensureOnStrand();
        spdlog::debug("Chunk loaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks[id] = std::move(*chunk);
    }

    asio::awaitable<void> WorldStreamer::unloadChunk(const ChunkID& id) 
    {
        asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

        if(cs.cancelled() != asio::cancellation_type::none)
        {
            spdlog::debug("[world-streamer] unloadChunk cancelled");
            co_return;
        }
        co_await _async_context.ensureOnStrand();
        spdlog::debug("Unloading chunk  ({};{};{})", id.x(), id.y(), id.z());

        if (!_loaded_chunks.contains(id)) co_return;

        const absl::flat_hash_set<astre::ecs::Entity> chunk = _chunk_entities[id];
        for (ecs::Entity e : chunk)
        {
            co_await _registry.destroyEntity(e);
        }

        co_await _async_context.ensureOnStrand();
        spdlog::debug("Chunk unloaded  ({};{};{})", id.x(), id.y(), id.z());
        _loaded_chunks.erase(id);
    }
    
    asio::awaitable<void> WorldStreamer::saveAll(asset::use_binary_t)
    {
        asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

        if(cs.cancelled() != asio::cancellation_type::none)
        {
            spdlog::debug("[world-streamer] saveAll use_binary cancelled");
            co_return;
        }
        co_await _async_context.ensureOnStrand();
        for (const auto& [_, chunk] : _loaded_chunks) {
            _archive.writeChunk(chunk, asset::use_binary);
        }
    }

    asio::awaitable<void> WorldStreamer::saveAll(asset::use_json_t)
    {
        asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

        if(cs.cancelled() != asio::cancellation_type::none)
        {
            spdlog::debug("[world-streamer] saveAll use_json cancelled");
            co_return;
        }
        co_await _async_context.ensureOnStrand();
        for (const auto& [_, chunk] : _loaded_chunks) {
            _archive.writeChunk(chunk, asset::use_json);
        }
    }
    
} // namespace astre::world
