#include <cassert>

#include <spdlog/spdlog.h>

#include "render/opengl/glsl_variable.hpp"
#include "render/opengl/opengl_texture.hpp"
#include "render/opengl/opengl_render_buffer_object.hpp"
#include "render/texture.hpp"
#include "render/render_buffer.hpp"

#include "render/opengl/opengl_frame_buffer_object.hpp"

namespace astre::render::opengl
{
    OpenGLFrameBufferObject::OpenGLFrameBufferObject(std::pair<unsigned int, unsigned int> resolution, std::vector<std::pair<std::size_t, FBOAttachment>> attachments)
    :  _FBO(0), _resolution(std::move(resolution))
    {
        if(generateBuffer() == false) 
        { 
            spdlog::error("OpenGL frame buffer object creation failed");
            return;
        }

        enable();

        processAttachments(attachments);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            spdlog::error("FBO validation failed");
            throw std::runtime_error("FBO validation failed");
        }

        disable();

        const auto check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] FBO generation failed, OpenGL error : {}", check.error());
        }
    }
    
    OpenGLFrameBufferObject::OpenGLFrameBufferObject(OpenGLFrameBufferObject && other)
    :   _FBO(other._FBO), _resolution(std::move(other._resolution))
    {
        other._FBO = 0;
    }

    OpenGLFrameBufferObject::~OpenGLFrameBufferObject()
    {
        if(good() == false)return;

        disable();
        removeBuffer();

        spdlog::info("OpenGL frame buffer object destroyed");
    }

    std::size_t OpenGLFrameBufferObject::ID() const
    {
        return _FBO;
    }

    bool OpenGLFrameBufferObject::good() const 
    {
        return _FBO != 0;
    }

    bool OpenGLFrameBufferObject::removeBuffer()
    {
        if(_FBO != 0)
        {
            spdlog::debug(std::format("OpenGL frame buffer object {} destroy" , _FBO));
            glDeleteFramebuffers(1, &_FBO);
            _FBO = 0;
            return true;
        }
        else 
        {
            spdlog::error(std::format("OpenGL frame buffer object {} destroy failed", _FBO) );
            return false;
        }
    }

    bool OpenGLFrameBufferObject::generateBuffer()
    {        
        spdlog::debug("Generating new OpenGL frame buffer object ... ");

        glGenFramebuffers(1, &_FBO);

        const auto check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] FBO generate buffer failed, OpenGL error : {}", check.error());
        }
        
        spdlog::debug(std::format("OpenGL frame buffer object {} created", _FBO));
        
        return good();
    }

    void OpenGLFrameBufferObject::disable() const
    {
        if(_FBO == 0)return;

        GLint currently_bound_FBO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currently_bound_FBO);
        
        if(currently_bound_FBO != (GLint)_FBO)
        {
            spdlog::warn("Disable OpenGL frame buffer object which is not currently bound");
            return;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    bool OpenGLFrameBufferObject::enable() const
    {
        if(_FBO == 0)
        {
            spdlog::error("Use of uninitialized OpenGL frame buffer object");
            return false;
        }

        // check if need of bind SSBO
        GLint currently_bound_FBO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currently_bound_FBO);

        // already binded
        if(currently_bound_FBO == (GLint)_FBO) return true;
        
        // try to bind
        glBindFramebuffer(GL_FRAMEBUFFER, _FBO);

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currently_bound_FBO);

        if(currently_bound_FBO != (GLint)_FBO)
        { 
            spdlog::error(std::format("FBO binding failed, currently bound: {}, should be {} ", 
                    currently_bound_FBO, _FBO));
                
            disable();
            return false;
        }
        
        return true;
    }

    const std::vector<std::size_t> & OpenGLFrameBufferObject::getTextures() const
    {
        return _attached_textures;
    }

    std::pair<unsigned int, unsigned int> OpenGLFrameBufferObject::getResolution() const
    {
        return _resolution;
    }

    void OpenGLFrameBufferObject::processAttachments(const std::vector<std::pair<std::size_t, FBOAttachment>> & attachments)
    {
        GLenum attachment_point;
        unsigned short color_index = 0;
        unsigned short depth_index = 0;
        unsigned short stencil_index = 0;
        std::vector<GLenum> draw_buffers;

        for(const auto & [id, att] : attachments)
        {
            assert(color_index < 8 && "More than 8 color attachments not supported");
            assert(depth_index < 1 && "Multiple depth attachment not supported");
            assert(stencil_index < 1 && "Multiple stencil attachment not supported");

            switch (att.point)
            {
                case FBOAttachment::Point::Color:
                    attachment_point = GL_COLOR_ATTACHMENT0 + (color_index++);
                    break;
                case FBOAttachment::Point::Depth:
                    attachment_point = GL_DEPTH_ATTACHMENT + (depth_index++);
                    break;
                case FBOAttachment::Point::Stencil:
                    attachment_point = GL_STENCIL_ATTACHMENT + (stencil_index++);
                    break;
            }

            if(att.type == FBOAttachment::Type::Texture)
            {
                _attached_textures.emplace_back(id);

                // attach texture to FBO
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_point, GL_TEXTURE_2D, id, 0);

                if(att.point == FBOAttachment::Point::Color)draw_buffers.emplace_back(attachment_point);
            }
            else if(att.type == FBOAttachment::Type::RenderBuffer)
            {
                _attached_render_buffers.emplace_back(id);
                
                // attach RBO to FBO
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment_point, GL_RENDERBUFFER, id);
            }
            else
            {
                spdlog::error("Unknown FBO attachment type");
                throw std::runtime_error("Unknown FBO attachment type");
            }
        }
        
        if(draw_buffers.size() > 0 )
        {
            glDrawBuffers(draw_buffers.size(), draw_buffers.data());
        }
        else 
        {
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }
    }
}