#include <spdlog/spdlog.h>

#include "asset/asset.hpp"
#include "asset/resource_tracker.hpp"


namespace astre::asset
{
    // ---------- Collectors ----------
    RequiredResources ResourceTracker::_collectFrom(const ecs::EntityDefinition& def) noexcept
    {
        RequiredResources out{};
        if (def.has_visual())
        {
            const auto& v = def.visual();
            _maybeInsert(out.shaders,       v.shader_name());
            _maybeInsert(out.vertexBuffers, v.vertex_buffer_name());
        }
        if (def.has_script())
        {
            _maybeInsert(out.scripts, def.script().name());
        }
        return out;
    }

RequiredResources ResourceTracker::_collectFrom(const ecs::Registry& reg, ecs::Entity e) noexcept
{
    RequiredResources out{};
    // Visual
    reg.runOnSingleWithComponents<ecs::VisualComponent>(e, [&](ecs::Entity, const ecs::VisualComponent& v){
        _maybeInsert(out.shaders,       v.shader_name());
        _maybeInsert(out.vertexBuffers, v.vertex_buffer_name());
    });
    // Script
    reg.runOnSingleWithComponents<ecs::ScriptComponent>(e, [&](ecs::Entity, const ecs::ScriptComponent& s){
        _maybeInsert(out.scripts, s.name());
    });
    return out;
}

// ---------- Ensure (async) ----------
asio::awaitable<void> ResourceTracker::_ensureLoaded(const RequiredResources& req)
{
    // Shaders
    for (const auto& sh : req.shaders) {
        co_await _ensureShader(sh);
    }
    // Vertex buffers
    for (const auto& vb : req.vertexBuffers) {
        co_await _ensureVertexBuffer(vb);
    }
    // Scripts
    for (const auto& sc : req.scripts) {
        co_await _ensureScript(sc);
    }
    co_return;
}

asio::awaitable<void> ResourceTracker::_ensureShader(std::string_view shader)
{
    if (_loaded_shaders.contains(shader)) co_return;

    // If renderer already has it, we’re done.
    if (auto found = _renderer.getShader(std::string(shader)); found.has_value())
    {
        _loaded_shaders.insert(std::string(shader));
        co_return;
    }

    const auto dir = _shaders_root / std::string(shader);
    auto ok = co_await asset::loadShaderFromDir(_renderer, dir);
    if (!ok)
    {
        spdlog::warn("[assets] failed to load shader '{}': {}", shader, dir.string());
        co_return;
    }
    _loaded_shaders.insert(std::string(shader));
    co_return;
}

asio::awaitable<void> ResourceTracker::_ensureVertexBuffer(std::string_view vb) 
{
    if (_loaded_vertex_buffers.contains(vb)) co_return;

    // If renderer already has it, we’re done.
    if (auto found = _renderer.getVertexBuffer(std::string(vb)); found.has_value())
    {
        _loaded_vertex_buffers.insert(std::string(vb));
        co_return;
    }

    bool loaded = false;
    if (!loaded)
    {
        spdlog::debug("[assets] vertex buffer '{}' not present and no loader provided.", vb);
        co_return;
    }
    _loaded_vertex_buffers.insert(std::string(vb));
    co_return;
}

asio::awaitable<void> ResourceTracker::_ensureScript(std::string_view script) 
{
    if (_loaded_scripts.contains(script)) co_return;

    // If runtime already has it, we’re done.
    if (auto found = _scripts.scriptLoaded(std::string(script)); found == true)
    {
        _loaded_scripts.insert(std::string(script));
        co_return;
    }

    const auto path = _scripts_root / (std::string(script) + ".lua");
    const bool ok = co_await asset::loadScript(_scripts, path);
    if (!ok)
    {
        spdlog::warn("[assets] failed to load script '{}': {}", script, path.string());
        co_return;
    }
    _loaded_scripts.insert(std::string(script));
    co_return;
}

} // namespace astre::asset