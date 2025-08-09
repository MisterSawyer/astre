#pragma once

#include "native/native.h"
#include <asio.hpp>

#include "gui/gui.hpp"
#include "render/render.hpp"

namespace astre::pipeline 
{
    asio::awaitable<void> renderGUIStats(
        gui::GUIService & gui, 
        float logic_fps, float logic_frame_time,
        const render::FrameStats & render_stats,
        render::RenderOptions & gbuffer_render_options, render::RenderOptions & shadow_map_render_options);
}