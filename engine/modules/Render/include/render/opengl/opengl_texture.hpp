#pragma once

#include <format>
#include <utility>

#include <GL/glew.h>
#include <spdlog/spdlog.h>

#include "render/texture.hpp"

#include "render/opengl/opengl_debug.hpp"

namespace astre::render::opengl
{
    GLenum textureFormatToOpenGLFormat(TextureFormat format);
    std::pair<GLenum, GLenum> getBaseFormatAndType(GLenum format);

    class OpenGLTexture
    {
        public:
            OpenGLTexture(std::pair<unsigned int, unsigned int> resolution, GLenum format = GL_RGBA);
            
            ~OpenGLTexture() = default;

            OpenGLTexture(OpenGLTexture && other) = default;

            std::size_t ID() const;

            bool good() const;

            bool enable() const;

            void disable() const;

        private:
            GLuint _ID;
            std::pair<unsigned int, unsigned int> _resolution;
    };
}