#pragma once

#include <filesystem>
#include <fstream>

#include "native/native.h"
#include <asio.hpp>

#include "render/render.hpp"

#include "asset/component_loader.hpp"
#include "asset/component_serializer.hpp"

namespace astre::asset
{
    struct use_json_t{};
    constexpr const use_json_t use_json;

    struct use_binary_t{};
    constexpr const use_binary_t use_binary;

    asio::awaitable<bool> loadVertexBuffersPrefabs(render::IRenderer & renderer);

    asio::awaitable<std::optional<std::size_t>> loadShaderFromFile(render::IRenderer & renderer, const std::filesystem::path shader_path);
    asio::awaitable<bool> loadShadersFromDir(render::IRenderer & renderer, const std::filesystem::path shader_path);
}