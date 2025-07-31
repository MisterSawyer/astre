#pragma once

#include <filesystem>
#include <fstream>

#include "native/native.h"
#include <asio.hpp>

#include "render/render.hpp"

#include "asset/entity_loader.hpp"
#include "asset/entity_serializer.hpp"

namespace astre::asset
{
    struct use_json_t{};
    constexpr use_json_t use_json;

    struct use_binary_t{};
    constexpr use_binary_t use_binary;

    asio::awaitable<bool> loadVertexBuffersPrefabs(render::IRenderer & renderer);

    asio::awaitable<std::optional<std::size_t>> loadShaderFromDir(render::IRenderer & renderer, const std::filesystem::path shader_dir);
}