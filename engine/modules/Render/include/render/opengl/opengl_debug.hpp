#pragma once

#include <expected>
#include <string>

#include <GL/glew.h>
#include <spdlog/spdlog.h>

namespace astre::render::opengl
{
    std::expected<void, std::string> checkOpenGLState();
}