#pragma once

#include <expected>
#include <string>

namespace astre::render::opengl
{
    std::expected<void, std::string> checkOpenGLState();
}