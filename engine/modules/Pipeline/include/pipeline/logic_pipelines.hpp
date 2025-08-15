#pragma once

#include "native/native.h"
#include <asio.hpp>

#include "math/math.hpp"
#include "ecs/ecs.hpp"
#include "render/render.hpp"
#include "world/world.hpp"

#include "pipeline/app_state.hpp"

namespace astre::pipeline 
{
    asio::awaitable<void> runPreECS(AppState & app_state, world::WorldStreamer & world_streamer, const math::Vec3 & load_position);

    asio::awaitable<void> runECS(ecs::Systems & systems, float dt, render::Frame & render_frame);
}