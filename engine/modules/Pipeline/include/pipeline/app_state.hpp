#pragma once

#include "process/process.hpp"
#include "window/window.hpp"
#include "render/render.hpp"
#include "input/input.hpp"
#include "gui/gui.hpp"

namespace astre::pipeline
{
    struct AppState
    {
        process::IProcess & process;
        window::IWindow & window;
        render::IRenderer & renderer;
        input::InputService & input;
        gui::GUIService & gui;
        script::ScriptRuntime & script;
    };
}