#include "render/opengl/opengl_shader_storage_buffer.hpp"

namespace astre::render::opengl
{
    OpenGLShaderStorageBuffer::OpenGLShaderStorageBuffer(unsigned int binding_point, std::size_t size, const void * data)
    :   _size(std::move(size)),
        _data(std::move(data)),
        _SSBO(0)
    {
        if(generateBuffer() == false) 
        { 
            spdlog::error("OpenGL shader storage buffer creation failed");
            return;
        }

        enable();
        setGPUAttributes(binding_point);
        copyDataToGPU();

        disable();

        const auto check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] SSBO validation buffer failed, OpenGL error : {}", check.error());
        }
    }
    
    OpenGLShaderStorageBuffer::OpenGLShaderStorageBuffer(OpenGLShaderStorageBuffer && other)
    :   _SSBO(other._SSBO),
        _size(std::move(other._size)),
        _data(std::move(other._data))
    {
        other._SSBO = 0;
        other._size = 0;
        other._data = nullptr;
    }

    OpenGLShaderStorageBuffer::~OpenGLShaderStorageBuffer()
    {
        if(good() == false)return;

        disable();
        removeBuffer();

        spdlog::info("OpenGL shader storage buffer destroyed");
    }

    std::size_t OpenGLShaderStorageBuffer::ID() const
    {
        return _SSBO;
    }

    bool OpenGLShaderStorageBuffer::good() const 
    {
        return _SSBO != 0;
    }

    void OpenGLShaderStorageBuffer::update(std::size_t size, const void * data)
    {
        _size = (GLsizeiptr)size;
        _data = data;
        copyDataToGPU();
    }

    bool OpenGLShaderStorageBuffer::removeBuffer()
    {
        if(_SSBO != 0)
        {
            spdlog::debug(std::format("OpenGL shader storage buffer destroy SSBO {}" , _SSBO));
            glDeleteBuffers(1, &_SSBO);
            _SSBO = 0;
            return true;
        }
        else 
        {
            spdlog::error(std::format("OpenGL shader storage buffer destroy SSBO {} failed", _SSBO) );
            return false;
        }
    }

    bool OpenGLShaderStorageBuffer::generateBuffer()
    {        
        spdlog::debug("Generating new OpenGL shader storage buffer ... ");

        glGenBuffers(1, &_SSBO);

        const auto check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] SSBO generate buffer failed, OpenGL error : {}", check.error());
        }

        spdlog::debug(std::format("OpenGL shader storage buffer SSBO {}", _SSBO));
        
        return good();
    }

    void OpenGLShaderStorageBuffer::disable() const
    {
        if(_SSBO == 0)return;

        GLint currently_bound_SSBO = 0;
        glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &currently_bound_SSBO);
        
        if(currently_bound_SSBO != (GLint)_SSBO)
        {
            spdlog::warn("Disable OpenGL shader storage buffer which is not currently bound");
            return;
        }
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    bool OpenGLShaderStorageBuffer::enable() const
    {
        if(_SSBO == 0)
        {
            spdlog::error("Use of uninitialized OpenGL shader storage buffer");
            return false;
        }

        // check if need of bind SSBO
        GLint currently_bound_SSBO = 0;
        glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &currently_bound_SSBO);

        // already binded
        if(currently_bound_SSBO == (GLint)_SSBO) return true;
        
        // try to bind
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _SSBO);

        glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &currently_bound_SSBO);

        if(currently_bound_SSBO != (GLint)_SSBO)
        { 
            spdlog::error(std::format("SSBO binding failed, currently bound: {}, should be {} ", 
                    currently_bound_SSBO, _SSBO));
                
            disable();
            return false;
        }
        
        return true;
    }

    bool OpenGLShaderStorageBuffer::setGPUAttributes(GLuint binding_point)
    {
        spdlog::debug(std::format("Setting shader storage buffer {} to be bound at {} binding point", _SSBO, binding_point));
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, _SSBO);

        return true;
    }

    bool OpenGLShaderStorageBuffer::copyDataToGPU() const
    {
        if(enable() == false)
        {
            spdlog::error("Cannot enable OpenGL shader storage buffer");
            return false;
        }

        glBufferData(GL_SHADER_STORAGE_BUFFER, _size, _data, GL_DYNAMIC_DRAW);
        
        return true;
    }
}