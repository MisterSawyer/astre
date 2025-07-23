#pragma once

#include <thread>

#include <GL/glew.h>
#ifdef WIN32
    #include <GL/wglew.h>
#endif

#include "native/native.h"
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include <absl/container/flat_hash_map.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "formatter/formatter.hpp"
#include "process/process.hpp"
#include "window/window.hpp"

#include "render/render_options.hpp"
#include "render/vertex.hpp"
#include "render/vertex_buffer.hpp"
#include "render/shader.hpp"
#include "render/shader_storage_buffer.hpp"
#include "render/frame_buffer_object.hpp"
#include "render/texture.hpp"
#include "render/render_buffer.hpp"

#include "render/opengl/opengl_debug.hpp"
#include "render/opengl/opengl_vertex_buffer.hpp"
#include "render/opengl/opengl_shader.hpp"
#include "render/opengl/opengl_shader_storage_buffer.hpp"
#include "render/opengl/opengl_frame_buffer_object.hpp"
#include "render/opengl/opengl_texture.hpp"
#include "render/opengl/opengl_render_buffer_object.hpp"

namespace astre::render::opengl
{

    class OpenGLRenderThreadContext
    {
        public:
        OpenGLRenderThreadContext(
            window::IWindow & window,
            native::opengl_context_handle oglctx);

        ~OpenGLRenderThreadContext();
                
        void join();

        const asio::strand<asio::io_context::executor_type> & getStrand() const;

        asio::awaitable<void> ensureOnStrand();
                
        void signalClose();

        bool running() const;

        native::device_context getDeviceContext();

        bool updateDeviceContext();

        private:
            void workerThread();

            asio::io_context _io_context;
            asio::executor_work_guard<asio::io_context::executor_type> _work_guard;
            asio::strand<asio::io_context::executor_type> _strand;

            window::IWindow & _window;
            native::device_context _device_context;
            native::opengl_context_handle _oglctx_handle;

            std::thread _worker_thread;
    };

    /**
     * @brief OpenGL `IRenderer` interface implementation
     */
    class OpenGLRenderer
    {
        public:
            OpenGLRenderer(window::IWindow & window, native::opengl_context_handle oglctx_handle);
            OpenGLRenderer(OpenGLRenderer && other);
            ~OpenGLRenderer();

            bool good() const;

            asio::awaitable<void> close();

            asio::awaitable<void> clearScreen(glm::vec4 color, std::optional<std::size_t> fbo);
            asio::awaitable<void> render(std::size_t vertex_buffer,
                std::size_t shader,
                ShaderInputs shader_inputs,
                RenderOptions options,
                std::optional<std::size_t> fbo);
                
            asio::awaitable<void> present();
            asio::awaitable<void> updateViewportSize(std::pair<unsigned int, unsigned int> resolution);
            std::pair<unsigned int, unsigned int> getViewportSize() const;
            asio::awaitable<void> enableVSync();
            asio::awaitable<void> disableVSync();

            asio::awaitable<std::optional<std::size_t>> createVertexBuffer(std::string name, const Mesh & mesh);
            asio::awaitable<bool> eraseVertexBuffer(std::size_t id);
            std::optional<std::size_t> getVertexBuffer(std::string name) const;

            asio::awaitable<std::optional<std::size_t>> createShader(std::string name, std::vector<std::string> vertex_code);
            asio::awaitable<std::optional<std::size_t>> createShader(std::string name, std::vector<std::string> vertex_code, std::vector<std::string> fragment_code);
            asio::awaitable<bool> eraseShader(std::size_t id);
            std::optional<std::size_t> getShader(std::string name) const;

            asio::awaitable<std::optional<std::size_t>> createShaderStorageBuffer(std::string name, unsigned int binding_point, const std::size_t size, const void * data);
            asio::awaitable<bool> eraseShaderStorageBuffer(std::size_t id);
            std::optional<std::size_t> getShaderStorageBuffer(std::string name) const;
            asio::awaitable<bool> updateShaderStorageBuffer(std::size_t id, const std::size_t size, const void * data);
            
            asio::awaitable<std::optional<std::size_t>> createFrameBufferObject(std::string name, std::pair<unsigned int, unsigned int> resolution, std::initializer_list<FBOAttachment> attachments);
            asio::awaitable<bool> eraseFrameBufferObject(std::size_t id);
            std::optional<std::size_t> getFrameBufferObject(std::string name) const;
            std::vector<std::size_t> getFrameBufferObjectTextures(std::size_t id) const;

            asio::awaitable<std::optional<std::size_t>> createTexture(std::string name, std::pair<unsigned int, unsigned int> resolution, TextureFormat format);
            asio::awaitable<bool> eraseTexture(std::size_t id);
            std::optional<std::size_t> getTexture(std::string name) const;

            void join();

        protected:

            template<class ImplType, class T, class ... Args>
            asio::awaitable<std::optional<std::size_t>> createInternalObject(
                absl::flat_hash_map<std::size_t, T> & object_map,
                absl::flat_hash_map<std::string, std::size_t> & name_map,
                std::string name,  Args && ... args)
            {
                if(good() == false)co_return std::nullopt;

                co_await _render_context->ensureOnStrand();

                if(name_map.contains(name))
                {
                    spdlog::warn(std::format("[t:{}] renderer object {} already exists", std::this_thread::get_id(), name));
                    co_return std::nullopt;
                }

                T obj(std::in_place_type<ImplType>, std::forward<Args>(args)...);

                const std::size_t id = obj->ID();

                if(object_map.contains(id))co_return std::nullopt;

                object_map.try_emplace(id, std::move(obj));
                name_map.try_emplace(std::move(name), id);

                co_return id;
            }

            template<class T>
            std::optional<std::size_t> getInternalObjectID(const absl::flat_hash_map<std::size_t, T> & object_map,
                const absl::flat_hash_map<std::string, std::size_t> & name_map,
                std::string name) const
            {
                if(good() == false)return std::nullopt;

                if(name_map.contains(name) == false)
                {
                    spdlog::warn(std::format("[t:{}] renderer object {} does not exist", std::this_thread::get_id(), name));
                    return std::nullopt;
                }
                
                const auto id = name_map.at(name);
                if(object_map.contains(id) == false)
                {
                    spdlog::warn(std::format("[t:{}] renderer object {} does not exist", std::this_thread::get_id(), id));
                    return std::nullopt;
                }
                return id;
            }

            template<class T>
            asio::awaitable<bool> eraseInternalObject(absl::flat_hash_map<std::size_t, T> & object_map, std::size_t id) const
            {
                if(good() == false)co_return false;

                co_await _render_context->ensureOnStrand();

                if(object_map.contains(id) == false)
                {
                    spdlog::warn(std::format("[t:{}] renderer object {} does not exist", std::this_thread::get_id(), id));
                    co_return false;
                }

                object_map.erase(id);

                spdlog::info(std::format("[t:{}] renderer object {} erased", std::this_thread::get_id(), id));

                co_return true;
            }

            void assignShaderInputs(const std::size_t & shader_ID, const ShaderInputs & shader_inputs);

        private:
            window::IWindow & _window;

            native::opengl_context_handle _oglctx_handle;

            std::pair<unsigned int, unsigned int> _viewport_resolution;

            absl::flat_hash_map<std::size_t, VertexBuffer> _vertex_buffers;
            absl::flat_hash_map<std::size_t, Shader> _shaders;
            absl::flat_hash_map<std::size_t, ShaderStorageBuffer> _shader_storage_buffers;
            absl::flat_hash_map<std::size_t, FrameBufferObject> _frame_buffer_objects;
            absl::flat_hash_map<std::size_t, Texture> _textures;
            absl::flat_hash_map<std::size_t, RenderBuffer> _rbos;

            absl::flat_hash_map<std::string, std::size_t> _vertex_buffer_names;
            absl::flat_hash_map<std::string, std::size_t> _shader_names;
            absl::flat_hash_map<std::string, std::size_t> _shader_storage_buffer_names;
            absl::flat_hash_map<std::string, std::size_t> _frame_buffer_object_names;
            absl::flat_hash_map<std::string, std::size_t> _textures_names;
            absl::flat_hash_map<std::string, std::size_t> _rbos_names;

            // Render thread initialized at the end of the constructor
            std::unique_ptr<OpenGLRenderThreadContext> _render_context; // dedicated single thread
    };
}