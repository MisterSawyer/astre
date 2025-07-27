#include "render/render.hpp"
#include "render/opengl/opengl.hpp"


namespace astre::render
{
    asio::awaitable<render::Renderer> createRenderer(window::IWindow & window)
    {
        native::opengl_context_handle oglctx_handle = co_await window.getProcess().registerOGLContext(window.getHandle(), 4, 0);
        co_return Renderer(std::in_place_type<opengl::OpenGLRenderer>, window, oglctx_handle);
    }
}

namespace astre::render::opengl
{

    // ----------------------------------------------------------------
    // OpenGLRenderThreadContext
    // ----------------------------------------------------------------

    OpenGLRenderThreadContext::OpenGLRenderThreadContext(window::IWindow & window, native::opengl_context_handle oglctx)
    :   _window(window), 
        _device_context(nullptr),
        _oglctx_handle(oglctx) 
    {
        start(&OpenGLRenderThreadContext::workerThread, this);
    }

    native::device_context OpenGLRenderThreadContext::getDeviceContext()
    {
        return _device_context;
    }

    bool OpenGLRenderThreadContext::updateDeviceContext()
    {
        // release old context
        #ifdef WIN32
            wglMakeCurrent(0, 0);
        #endif

        if(_device_context != nullptr)
        {
            if(_window.releaseDeviceContext(_device_context) == false)
            {
                spdlog::error("Cannot release OpenGL device context");
                return false;
            }
        }
        
        // acquire new context
        _device_context = _window.acquireDeviceContext();

        // bind context
        bool binding_success = true;
        #ifdef WIN32
            binding_success = wglMakeCurrent(_device_context, _oglctx_handle) == TRUE;
        #endif

        if(binding_success == false)
        {
            spdlog::error("Cannot activate OpenGL context");
            return false;
        }

        return true;
    }

    void OpenGLRenderThreadContext::workerThread()
    {
        #ifdef WIN32
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        #endif

        spdlog::debug("[opengl] Render thread started");
                
        _device_context = _window.acquireDeviceContext();
                
        // bind context
        bool binding_success = true;

        #ifdef WIN32
            binding_success = wglMakeCurrent(_device_context, _oglctx_handle) == TRUE;
        #endif

        if(binding_success == false)
        {
            spdlog::error("Cannot bind OpenGL context");
            return;
        }

        // Enable various OpenGL features
        // inside renderer thread
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);     // Enable culling
        glCullFace(GL_BACK);       // Cull back faces (default)
        glFrontFace(GL_CCW);       // Counter-clockwise is front (default)

        try
        {
            run();
        }
        catch(const std::exception & e)
        {
            spdlog::error("[render] OpenGL render thread exception: {}", e.what());
        }
    
        // unbind context
        #ifdef WIN32
            wglMakeCurrent(0, 0);
        #endif

        if(_device_context != nullptr)
        {
            if(_window.releaseDeviceContext(_device_context) == false)
            {
                spdlog::error("Cannot release OpenGL device context");
            }
        }

        spdlog::debug("[opengl]  Render thread ended");
    }

    // ----------------------------------------------------------------
    // OpenGLRenderer
    // ----------------------------------------------------------------

    OpenGLRenderer::OpenGLRenderer(window::IWindow & window, native::opengl_context_handle oglctx_handle)
    :   _window(window),
        
        _oglctx_handle(oglctx_handle),
        _vertex_buffers(),
        _shaders(),

        _viewport_resolution(0, 0),

        _render_context(std::make_unique<OpenGLRenderThreadContext>(_window, _oglctx_handle))
    {
        spdlog::info("[render] OpenGL renderer created");
    }

    OpenGLRenderer::OpenGLRenderer(OpenGLRenderer && other)
    :   _window(other._window),
        
        _render_context(std::move(other._render_context)),
        _oglctx_handle(std::move(other._oglctx_handle)),

        _vertex_buffers(std::move(other._vertex_buffers)),
        _shaders(std::move(other._shaders)),

        _viewport_resolution(std::move(other._viewport_resolution))
    {
        other._oglctx_handle = nullptr;
    }


    OpenGLRenderer::~OpenGLRenderer()
    {
        join();
        
        assert(_render_context == nullptr && "OpenGL context should be closed before destruction" );
        assert(_oglctx_handle == nullptr && "OpenGL context should be closed before destruction" );
    }

    void OpenGLRenderer::join()
    {
        if(_render_context == nullptr) return;

        spdlog::debug("[opengl] OpenGL renderer join");
        
        _render_context->co_spawn(close());
        
        // wait for render thread to finish
        _render_context->join();

        // destroy render thread context
        _render_context.reset();
    }

    asio::awaitable<void> OpenGLRenderer::close()
    {
        if(good() == false)co_return;

        co_await _render_context->ensureOnStrand();

        spdlog::debug("[opengl] close");

        // set render_context to closed such that we won't accept any new 
        // rendering tasks
        _render_context->close();

        _vertex_buffers.clear();
        _shaders.clear();
        _shader_storage_buffers.clear();
        _frame_buffer_objects.clear();
        _textures.clear();
        _rbos.clear();

        // unbind context before unregistering in winapi process
        #ifdef WIN32
            wglMakeCurrent(0, 0);
        #endif 
        native::opengl_context_handle oglctx_handle = _oglctx_handle;
        _oglctx_handle = nullptr;
        co_await _window.getProcess().unregisterOGLContext(oglctx_handle);

        co_return;
    }

    bool OpenGLRenderer::good() const
    {
        return  _oglctx_handle != nullptr && _render_context != nullptr &&
                _render_context != nullptr && _render_context->running();
    }

    asio::awaitable<void> OpenGLRenderer::enableVSync()
    {
        if(good() == false)co_return;

        co_await _render_context->ensureOnStrand();

        #ifdef WIN32
            wglSwapIntervalEXT(1);
        #endif

        co_return;
    }

    asio::awaitable<void> OpenGLRenderer::disableVSync()
    {
        if(good() == false)co_return;

        co_await _render_context->ensureOnStrand();

        #ifdef WIN32
            wglSwapIntervalEXT(0);
        #endif

        co_return;
    }

    asio::awaitable<std::optional<std::size_t>> OpenGLRenderer::createVertexBuffer(std::string name,  const Mesh & mesh)
    {
        co_return co_await (createInternalObject<OpenGLVertexBuffer>(
            _vertex_buffers, _vertex_buffer_names, std::move(name),
            mesh.indices, mesh.vertices));
    }

    asio::awaitable<bool> OpenGLRenderer::eraseVertexBuffer(std::size_t id)
    {
        co_return co_await eraseInternalObject(_vertex_buffers, std::move(id));
    }

    std::optional<std::size_t> OpenGLRenderer::getVertexBuffer(std::string name) const
    { 
        return getInternalObjectID(_vertex_buffers, _vertex_buffer_names, std::move(name));
    }


    asio::awaitable<std::optional<std::size_t>> OpenGLRenderer::createShader(std::string name, std::vector<std::string> vertex_code)
    {
        co_return co_await (createInternalObject<OpenGLShader>(
            _shaders, _shader_names, std::move(name), std::move(vertex_code)));
    }

    asio::awaitable<std::optional<std::size_t>> OpenGLRenderer::createShader(std::string name, std::vector<std::string> vertex_code, std::vector<std::string> fragment_code)
    {
        co_return co_await (createInternalObject<OpenGLShader>(
            _shaders, _shader_names, std::move(name), std::move(vertex_code),
            std::move(fragment_code)));
    }

    asio::awaitable<bool> OpenGLRenderer::eraseShader(std::size_t id)
    {
        co_return co_await eraseInternalObject(_shaders, std::move(id));
    }

    std::optional<std::size_t> OpenGLRenderer::getShader(std::string name) const
    {
        return getInternalObjectID(_shaders, _shader_names, std::move(name));
    }


    asio::awaitable<std::optional<std::size_t>> OpenGLRenderer::createShaderStorageBuffer(
                std::string name,
                unsigned int binding_point,
                const std::size_t size,
                const void * data)
    {
        co_return co_await (createInternalObject<OpenGLShaderStorageBuffer>(
            _shader_storage_buffers, _shader_storage_buffer_names, std::move(name),
            binding_point, std::move(size), std::move(data)));
    }

    asio::awaitable<bool> OpenGLRenderer::eraseShaderStorageBuffer(std::size_t id)
    {
        co_return co_await eraseInternalObject(_shader_storage_buffers, std::move(id));
    }

    std::optional<std::size_t> OpenGLRenderer::getShaderStorageBuffer(std::string name) const
    {
        return getInternalObjectID(_shader_storage_buffers, _shader_storage_buffer_names, std::move(name));
    }
    
    asio::awaitable<bool> OpenGLRenderer::updateShaderStorageBuffer(std::size_t id, const std::size_t size, const void * data)
    {
        if(good() == false)co_return false;
        
        co_await _render_context->ensureOnStrand();
        
        if(_shader_storage_buffers.contains(id) == false)
        {
            spdlog::warn(std::format("Shader storage buffer {} does not exist", id));
            co_return false;
        }

        _shader_storage_buffers.at(id)->update(std::move(size), std::move(data));
        co_return true;
    }


    asio::awaitable<std::optional<std::size_t>> OpenGLRenderer::createFrameBufferObject(std::string name, std::pair<unsigned int, unsigned int> resolution, std::initializer_list<FBOAttachment> attachments)
    {
        if(good() == false)co_return false;
        
        co_await _render_context->ensureOnStrand();

        std::vector<std::pair<std::size_t, FBOAttachment>> constructed_attachments;

        // create all attachments
        for(const auto & att : attachments)
        {
            if(att.type == FBOAttachment::Type::Texture)
            {
                // create unnamed texture for FBO
                Texture tex(std::in_place_type<OpenGLTexture>, resolution, textureFormatToOpenGLFormat(att.format));
                if(tex->good() == false)
                {
                    spdlog::warn("Failed to construct texture for FBO attachment");
                    co_return std::nullopt;
                }

                const auto id = tex->ID();
                _textures.try_emplace(id, std::move(tex));
                constructed_attachments.emplace_back(id, att);
   
            }
            else if(att.type == FBOAttachment::Type::RenderBuffer)
            {
                // create unnamed RBO for FBO
                RenderBuffer rbo(std::in_place_type<OpenGLRenderBufferObject>, resolution, textureFormatToOpenGLFormat(att.format));
                if(rbo->good() == false)
                {
                    spdlog::warn("Failed to construct render buffer for FBO attachment");
                    co_return std::nullopt;
                }

                const auto id = rbo->ID();
                _rbos.try_emplace(id, std::move(rbo));
                constructed_attachments.emplace_back(id, att);
            }
            else
            {
                spdlog::warn("Unknown FBO attachment type");
                co_return std::nullopt;
            }
        }
        // create FBO
        co_return co_await (createInternalObject<OpenGLFrameBufferObject>(
            _frame_buffer_objects, _frame_buffer_object_names, std::move(name),
            std::move(resolution), std::move(constructed_attachments)));
    }

    asio::awaitable<bool> OpenGLRenderer::eraseFrameBufferObject(std::size_t id)
    {
        co_return co_await eraseInternalObject(_frame_buffer_objects, std::move(id));
    }

    std::optional<std::size_t> OpenGLRenderer::getFrameBufferObject(std::string name) const
    {

        return getInternalObjectID(_frame_buffer_objects, _frame_buffer_object_names, std::move(name));
    }

    std::vector<std::size_t> OpenGLRenderer::getFrameBufferObjectTextures(std::size_t id) const 
    {
        if(good() == false) return {};
        auto it = _frame_buffer_objects.find(id);
        if(it == _frame_buffer_objects.end()) return {};
        return it->second->getTextures();
    }

    asio::awaitable<std::optional<std::size_t>> OpenGLRenderer::createTexture(std::string name, std::pair<unsigned int, unsigned int> resolution, TextureFormat format)
    {
        co_return co_await (createInternalObject<OpenGLTexture>(
            _textures, _textures_names, std::move(name),
            std::move(resolution), textureFormatToOpenGLFormat(format)));
    }

    asio::awaitable<bool> OpenGLRenderer::eraseTexture(std::size_t id)
    {
        co_return co_await eraseInternalObject(_textures, std::move(id));
    }

    std::optional<std::size_t> OpenGLRenderer::getTexture(std::string name) const
    {
        return getInternalObjectID(_textures, _textures_names, std::move(name));
    }


    void OpenGLRenderer::assignShaderInputs(const std::size_t & shader_ID, const ShaderInputs & shader_inputs)
    {
        for(const auto & [name, val] : shader_inputs.in_bool)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }

        for(const auto & [name, val] : shader_inputs.in_int)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }
        for(const auto & [name, val] : shader_inputs.in_float)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }

        for(const auto & [name, val] : shader_inputs.in_vec2)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }
        for(const auto & [name, val] : shader_inputs.in_vec3)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }
        for(const auto & [name, val] : shader_inputs.in_vec4)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }

        for(const auto & [name, val] : shader_inputs.in_mat2)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }
        for(const auto & [name, val] : shader_inputs.in_mat3)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }
        for(const auto & [name, val] : shader_inputs.in_mat4)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }
        for(const auto & [name, val] : shader_inputs.in_mat4_array)
        {
            _shaders.at(shader_ID)->setUniform(name, val);
        }
        
        // SSBO
        for(const auto & storage_buffer : shader_inputs.storage_buffers)
        {
            auto shader_storage_buffer_it = _shader_storage_buffers.find(storage_buffer);
            if(shader_storage_buffer_it == _shader_storage_buffers.end()){
                spdlog::warn("Rendering: Shader storage buffer not found");
                continue;
            }
            if(shader_storage_buffer_it->second->enable() == false)
            {
                spdlog::error("Cannot enable shader storage buffer");
                continue;
            }
        }

        // Samplers
        unsigned int sampler_unit = 0;

        for(const auto & [name, sampler] : shader_inputs.in_samplers )
        {
            if(_textures.contains(sampler) == false)
            {
                spdlog::warn(std::format("Texture {} does not exist", name));
                continue;
            }

            _shaders.at(shader_ID)->setUniform(name, sampler_unit++, *_textures.at(sampler));
        }

        bool success = true;
        std::vector<ITexture*> in_textures;
        for(const auto & [name, samplers] : shader_inputs.in_samplers_array)
        {
            for(unsigned int i = 0; i < samplers.size(); ++i)
            {
                if(_textures.contains(samplers[i]) == false)
                {
                    spdlog::warn(std::format("Texture {} does not exist", name));
                    success = false;
                    continue;
                }
                else
                {
                    in_textures.emplace_back(_textures.at(samplers[i]).get());
                }
            }
            if(!success)continue;

            _shaders.at(shader_ID)->setUniform(name, sampler_unit, in_textures);
            sampler_unit += in_textures.size();
        }
    }

    asio::awaitable<void> OpenGLRenderer::clearScreen(math::Vec4 color, std::optional<std::size_t> fbo)
    {
        if(good() == false)co_return;

        co_await _render_context->ensureOnStrand();

        if(fbo)
        {
            if(_frame_buffer_objects.contains(*fbo) == false)
            {
                spdlog::error("Frame buffer object not found");
                co_return;
            }
            if(_frame_buffer_objects.at(*fbo)->enable() == false)
            {
                spdlog::error("Cannot enable frame buffer object");
                co_return;
            }
            const auto & fbo_obj_ref = _frame_buffer_objects.at(*fbo);
            glViewport(0, 0, (GLsizei)fbo_obj_ref->getResolution().first,
                 (GLsizei)fbo_obj_ref->getResolution().second);
        }
        else
        {
            glViewport(0, 0, (GLsizei)_viewport_resolution.first, 
                (GLsizei)_viewport_resolution.second);
        }

        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(fbo)
        {
            _frame_buffer_objects.at(*fbo)->disable();
        }

        co_return;
    }

    asio::awaitable<void> OpenGLRenderer::render(
            std::size_t vertex_buffer,
            std::size_t shader,
            ShaderInputs shader_inputs,
            RenderOptions options,
            std::optional<std::size_t> fbo)
    {
        if(good() == false)co_return;

        co_await _render_context->ensureOnStrand();

        if(fbo)
        {
            if(_frame_buffer_objects.contains(*fbo) == false)
            {
                spdlog::error("Frame buffer object not found");
                co_return;
            }
            if(_frame_buffer_objects.at(*fbo)->enable() == false)
            {
                spdlog::error("Cannot enable frame buffer object");
                co_return;
            }
            const auto & fbo_obj_ref = _frame_buffer_objects.at(*fbo);
            glViewport(0, 0, (GLsizei)fbo_obj_ref->getResolution().first,
                 (GLsizei)fbo_obj_ref->getResolution().second);
        }
        else
        {
            glViewport(0, 0, (GLsizei)_viewport_resolution.first, 
                (GLsizei)_viewport_resolution.second);
        }

        auto shader_it = _shaders.find(shader);
        if(shader_it == _shaders.end()){
            spdlog::warn("Rendering: Shader not found");
            co_return;
        }

        auto vertex_buffer_it = _vertex_buffers.find(vertex_buffer);
        if(vertex_buffer_it == _vertex_buffers.end()){
            spdlog::warn("Rendering: Vertex buffer not found");
            co_return;
        }


        if(shader_it->second->enable() == false){
            spdlog::error("Cannot enable shader program");
            co_return;
        }

        if(vertex_buffer_it->second->enable() == false){
            spdlog::error("Cannot enable vertex buffer");
            co_return;
        }

        assignShaderInputs(shader, shader_inputs);

        if(options.mode == RenderMode::Wireframe)glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        if(options.polygon_offset)
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(options.polygon_offset->factor, options.polygon_offset->units);
        }

        glDrawElements(GL_TRIANGLES, (GLsizei)vertex_buffer_it->second->numberOfElements(), GL_UNSIGNED_INT, nullptr);
        
        if(options.polygon_offset)
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        if(options.mode == RenderMode::Wireframe)glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // restore default


        if(fbo)
        {
            _frame_buffer_objects.at(*fbo)->disable();
        }

        co_return;
    }

    asio::awaitable<void> OpenGLRenderer::present()
    {
        if(good() == false)co_return;

        co_await _render_context->ensureOnStrand();

        glFlush();

        #ifdef WIN32
            SwapBuffers(_render_context->getDeviceContext());
        #endif

        co_return;
    }

    asio::awaitable<void> OpenGLRenderer::updateViewportSize(unsigned int width, unsigned int height)
    {
        if(good() == false)co_return;

        co_await _render_context->ensureOnStrand();
        
        _viewport_resolution = std::make_pair(width, height);

        // if the OpenGL context depends on a HDC that may change
        // (e.g. due to window resize, DPI change, or re-creation), 
        // then we must re-acquire the HDC (_device_context) from the HWND before calling glViewport()
        // or any other GL function that implicitly touches the current drawable surface
        _render_context->updateDeviceContext();

        co_return;
    }

    std::pair<unsigned int, unsigned int> OpenGLRenderer::getViewportSize() const
    {
        return _viewport_resolution;
    }
}