#pragma once

#include "native/native.h"
#include <asio.hpp>

#include "ecs/ecs.hpp"
#include "render/render.hpp"

namespace astre::pipeline 
{
    asio::awaitable<void> runECS(ecs::Systems & systems, float dt, render::Frame & render_frame);
}