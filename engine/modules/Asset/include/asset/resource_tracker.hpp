#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <asio.hpp>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "render/render.hpp"
#include "script/script.hpp"
#include "ecs/ecs.hpp"

#include "asset/asset.hpp"

namespace astre::asset 
{
    struct RequiredResources
    {
        absl::flat_hash_set<std::string> shaders;       // logical shader “folder” names (e.g. "deferred_shader")
        absl::flat_hash_set<std::string> vertexBuffers; // logical vertex buffer names (e.g. "cube_prefab")
        absl::flat_hash_set<std::string> scripts;       // logical script names (no extension, e.g. "player_script")
    };

    class ResourceTracker {
    public:
        ResourceTracker(render::IRenderer& renderer,
                        script::ScriptRuntime& scriptRuntime,
                        std::filesystem::path shadersRoot,
                        std::filesystem::path scriptsRoot) noexcept
            : _renderer(renderer)
            , _scripts(scriptRuntime)
            , _shaders_root(std::move(shadersRoot))
            , _scripts_root(std::move(scriptsRoot))
        {}

        // Convenience wrappers
        asio::awaitable<void> ensureFor(const proto::ecs::EntityDefinition& def) {
            co_await _ensureLoaded(_collectFrom(def));
        }
        asio::awaitable<void> ensureFor(const ecs::Registry& reg, ecs::Entity e) {
            co_await _ensureLoaded(_collectFrom(reg, e));
        }

        const absl::flat_hash_set<std::string> & getLoadedShaders() const { return _loaded_shaders; }
        const absl::flat_hash_set<std::string> & getLoadedVertexBuffers() const { return _loaded_vertex_buffers; }
        const absl::flat_hash_set<std::string> & getLoadedScripts() const { return _loaded_scripts; }

    private:
        // Fast collectors (pure, no I/O)
        static RequiredResources _collectFrom(const proto::ecs::EntityDefinition& def) noexcept;
        static RequiredResources _collectFrom(const ecs::Registry& reg, ecs::Entity e) noexcept;

        // Ensure everything in 'req' is present in renderer/runtime
        asio::awaitable<void> _ensureLoaded(const RequiredResources& req);

        // helpers
        asio::awaitable<void> _ensureShader(std::string_view shader);
        asio::awaitable<void> _ensureVertexBuffer(std::string_view vb);
        asio::awaitable<void> _ensureScript(std::string_view script);


        static void _maybeInsert(absl::flat_hash_set<std::string>& set, std::string_view s) noexcept {
            if (!s.empty()) set.insert(std::string{s});
        }

        render::IRenderer& _renderer;
        script::ScriptRuntime& _scripts;

        std::filesystem::path _shaders_root;
        std::filesystem::path _scripts_root;

        absl::flat_hash_set<std::string> _loaded_shaders;
        absl::flat_hash_set<std::string> _loaded_vertex_buffers;
        absl::flat_hash_set<std::string> _loaded_scripts;
    };

} // namespace astre::asset
