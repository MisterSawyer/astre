# Loader

Uploads asset definitions (protobuf messages) into runtime systems:
shaders and meshes into the renderer, scripts into the script runtime,
entities into the ECS registry.

This module is Stage 3 of the asset pipeline. Stage 1 import lives in `File`;
Stage 2 cache/streaming lives in `Asset`; loaders own the runtime residency set
and sync from a streamer's current keys.

## Concepts

- `AssetDefinition<Def>`: a protobuf message with a `name()`.
- `LoadSink<L, Def>` / `LoaderOf<L, Def>`: `L.load(const Def&) -> asio::awaitable<bool>`.
- `DefinitionSource<S, Key, Def>`: `S.read(Key) -> const Def*`.

## Loaders

| Loader | Definition | Target |
|---|---|---|
| `ShaderLoader` | `proto::render::ShaderDefinition` | `IRenderer::createShader` |
| `MeshLoader` | `proto::render::MeshDefinition` | `IRenderer::createVertexBuffer` and built-in prefabs |
| `ScriptLoader` | `proto::script::ScriptDefinition` | `script::ScriptRuntime::loadScript` |
| `EntityLoader` | `proto::ecs::EntityDefinition` | `ecs::Registry` |
| `ChunkLoader` | `proto::file::WorldChunk` | `EntityLoader` per chunk entity |
| `EntitySerializer` | registry entity | `EntityDefinition` |

## Sync

Streamers expose `keys()` and `read(key)`. Runtime code calls the matching
loader's `sync(keys, streamer)` so the loader can unload stale runtime assets
and load new ones while keeping its own loaded set.

```cpp
asset::ShaderStreamer shaders(process);
co_await shaders.stream(files);
co_await app_state.loaders.shader_loader.sync(shaders.keys(), shaders);
```

Meshes are the exception today: built-in meshes load through
`MeshLoader::loadPrefabs()`.
