#pragma once

#include <filesystem>
#include <fstream>

#include "native/native.h"
#include <asio.hpp>

#include "render/render.hpp"
#include "script/script.hpp"

#include "asset/resource_tracker.hpp"

namespace astre::asset
{
    asio::awaitable<bool> loadVertexBuffersPrefabs(render::IRenderer & renderer);

    asio::awaitable<std::optional<std::size_t>> loadShaderFromDir(render::IRenderer & renderer, const std::filesystem::path shader_dir);

    asio::awaitable<bool> loadScript(script::ScriptRuntime & runtime, const std::filesystem::path path);
}