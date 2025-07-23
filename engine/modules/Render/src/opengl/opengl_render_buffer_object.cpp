#include "render/opengl/opengl_render_buffer_object.hpp"

namespace astre::render::opengl
{
    OpenGLRenderBufferObject::OpenGLRenderBufferObject(std::pair<unsigned int, unsigned int> resolution, GLenum format)
    :   _resolution(std::move(resolution))
    {
        glGenRenderbuffers(1, &_ID);
        if(_ID == 0)
        {
            spdlog::error("Failed to generate OpenGL Render Buffer Object");
            return;
        }

        glBindRenderbuffer(GL_RENDERBUFFER, _ID);
        glRenderbufferStorage(GL_RENDERBUFFER, format, _resolution.first, _resolution.second);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
            
    std::size_t OpenGLRenderBufferObject::ID() const
    {
        return _ID;
    }

    bool OpenGLRenderBufferObject::good() const
    {
        return _ID != 0;
    }


    bool OpenGLRenderBufferObject::enable() const
    {
        if(good() == false)return false;

        glBindRenderbuffer(GL_RENDERBUFFER, _ID);
        return true;
    }
            

    void OpenGLRenderBufferObject::disable() const
    {
        if(good() == false)return;

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
}