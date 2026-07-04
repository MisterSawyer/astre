#pragma once

#include "process/process.hpp"
#include "window/window.hpp"
#include "render/render.hpp"

// services
#include "input/input.hpp"
#include "gui/gui.hpp"
#include "script/script.hpp"

#include "ecs/ecs.hpp"
#include "asset/asset.hpp"
#include "loader/loader.hpp"

namespace astre::pipeline
{

    struct AppStreamers
    {
        asset::WorldStreamer world_streamer;
        asset::ShaderStreamer shader_streamer;
        asset::ScriptStreamer script_streamer;
    };

    struct AppLoaders
    {
        AppLoaders(render::IRenderer & renderer,
                   script::ScriptRuntime & script_runtime,
                   ecs::Registry & registry)
        :   mesh_loader(renderer),
            shader_loader(renderer),
            script_loader(script_runtime),
            entity_loader(registry),
            chunk_loader(entity_loader)
        {}

        // chunk_loader holds a reference to entity_loader (a sibling member),
        // so this struct must never be copied or moved.
        AppLoaders(const AppLoaders &) = delete;
        AppLoaders & operator=(const AppLoaders &) = delete;
        AppLoaders(AppLoaders &&) = delete;
        AppLoaders & operator=(AppLoaders &&) = delete;

        loader::MeshLoader mesh_loader;
        loader::ShaderLoader shader_loader;
        loader::ScriptLoader script_loader;
        loader::EntityLoader entity_loader;
        loader::ChunkLoader chunk_loader;   // references entity_loader; declared after it
    };

    struct AppState
    {
        process::IProcess & process;
        window::IWindow & window;
        render::IRenderer & renderer;
        input::InputService & input;
        gui::GUIService & gui;
        script::ScriptRuntime & script;

        ecs::Registry & registry;
        ecs::Systems & systems;

        AppLoaders & loaders;
        AppStreamers & streamers;
    };
}