#include "render/opengl/opengl_debug.hpp"

namespace astre::render::opengl
{
    std::expected<void, std::string> checkOpenGLState()
    {
        switch(glGetError())
        {
            case GL_INVALID_VALUE:
            return std::unexpected("Invalid opengl object");

            case GL_INVALID_OPERATION:
            return std::unexpected("Invalid operation");

            case GL_INVALID_FRAMEBUFFER_OPERATION:
            return std::unexpected("The framebuffer object is not complete");

            case GL_OUT_OF_MEMORY:
            return std::unexpected("OpenGL is out of memory");

            case GL_STACK_UNDERFLOW:
            return std::unexpected("An attempt has been made to perform an operation that would cause an internal stack to underflow");

            case GL_STACK_OVERFLOW:
            return std::unexpected("An attempt has been made to perform an operation that would cause an internal stack to overflow");

            case GL_NO_ERROR:
            return {};

            default:
            return std::unexpected("Unidentified OpenGL problem");
        }
    }
}