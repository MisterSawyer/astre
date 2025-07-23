#pragma once

#include "type/type.hpp"

#include "render/render_options.hpp"

#include "render/vertex.hpp"
#include "render/vertex_buffer.hpp"

#include "render/shader.hpp"
#include "render/shader_storage_buffer.hpp"

#include "render/frame_buffer_object.hpp"
#include "render/texture.hpp"


namespace astre::render
{
    class IRenderer : public type::InterfaceBase
    {
    public:
        virtual ~IRenderer() = default;

        /**
         * @brief Check if renderer is properly initialized
         * 
         * @return `true` if renderer is properly initialized and can render, `false` if renderer is closed or not initialized
         */
        virtual bool good() const = 0;
        
        /**
         * @brief Close the renderer
         * notifies process
         * 
         * @return asio::awaitable<void> 
         */
        virtual asio::awaitable<void> close() = 0;

        /**
         * @brief Wait for the renderer to finish rendering and close
         * 
         */
        virtual void join() = 0;

        /**
         * @brief Clear the screen with a color
         * 
         * @param color Color to clear the screen with
         * @return asio::awaitable<void> 
         */
        virtual asio::awaitable<void> clearScreen(glm::vec4 color, std::optional<std::size_t> fbo = std::nullopt) = 0;

        /**
         * @brief Render a vertex buffer using a specified shader.
         * 
         * This function enables a frame buffer object (FBO) if provided, sets the viewport,
         * and renders the specified vertex buffer with the given shader. It applies shader inputs,
         * and supports rendering options such as wireframe mode and polygon offset.
         * 
         * @param vertex_buffer ID of the vertex buffer to render.
         * @param shader ID of the shader to use for rendering.
         * @param shader_inputs Inputs to be passed to the shader.
         * @param options Options for rendering, including rendering mode and polygon offset.
         * @param fbo Optional frame buffer object to render into. If not provided, rendering is done to the default framebuffer.
         * 
         * @return asio::awaitable<void> 
         */
        virtual asio::awaitable<void> render(
                std::size_t vertex_buffer,
                std::size_t shader,
                ShaderInputs shader_inputs = ShaderInputs{},
                RenderOptions options = RenderOptions{},
                std::optional<std::size_t> fbo = std::nullopt) = 0;
        
        /**
         * @brief Present the rendered frame
         * 
         * @return asio::awaitable<void> 
         */
        virtual asio::awaitable<void> present() = 0;

        /**
         * @brief Update the viewport
         * 
         * @return asio::awaitable<void> 
         */
        virtual asio::awaitable<void> updateViewportSize(unsigned int width, unsigned int height) = 0;

        virtual std::pair<unsigned int, unsigned int> getViewportSize() const = 0;
        
        /**
         * @brief Enable VSync
         * 
         * @return asio::awaitable<void> 
         */
        virtual asio::awaitable<void> enableVSync() = 0;

        /**
         * @brief Disable VSync
         * 
         * @return asio::awaitable<void> 
         */
        virtual asio::awaitable<void> disableVSync() = 0;

        /**
         * @brief Construct a vertex buffer object (VBO) for rendering a mesh.
         * 
         * @param name a unique name for the VBO
         * @param mesh the mesh data
         * 
         * @return ID of the VBO, or std::nullopt if failed
         * 
         * @note This function must be called on the render thread strand.
         */
        virtual asio::awaitable<std::optional<std::size_t>> createVertexBuffer(std::string name, const Mesh & mesh) = 0;

        /**
         * @brief Erase a vertex buffer object (VBO) by ID.
         * 
         * @param id ID of the VBO to erase.
         * 
         * @return `true` if the VBO was successfully erased, `false` otherwise.
         * 
         * @note This function must be called on the render thread strand.
         */
        virtual asio::awaitable<bool> eraseVertexBuffer(std::size_t id) = 0;

        /**
         * @brief Get the ID of a vertex buffer object (VBO) by name.
         * 
         * @param name Name of the VBO.
         * 
         * @return ID of the VBO, or std::nullopt if not found.
         * 
         * @note This function must be called on the render thread strand.
         */
        virtual std::optional<std::size_t> getVertexBuffer(std::string name) const = 0;

        /**
         * @brief Construct a new shader object with the given name and vertex code
         * @param name Name of the shader
         * @param vertex_code Vertex shader code
         * @return ID of the newly created shader object, or std::nullopt if the shader could not be created
         */
        virtual asio::awaitable<std::optional<std::size_t>> createShader(std::string name, std::vector<std::string> vertex_code) = 0;

        /**
         * @brief Construct a new shader object with the given name and vertex and fragment code
         * @param name Name of the shader
         * @param vertex_code Vertex shader code
         * @param fragment_code Fragment shader code
         * @return ID of the newly created shader object, or std::nullopt if the shader could not be created
         */
        virtual asio::awaitable<std::optional<std::size_t>> createShader(std::string name, std::vector<std::string> vertex_code, std::vector<std::string> fragment_code) = 0;

        /**
         * @brief Erase a shader object by ID
         * @param id ID of the shader object to erase
         * @return `true` if the shader object was successfully erased, `false` otherwise
         */
        virtual asio::awaitable<bool> eraseShader(std::size_t id) = 0;

        /**
         * @brief Get the ID of a shader object by name
         * @param name Name of the shader object
         * @return ID of the shader object, or std::nullopt if not found
         */
        virtual std::optional<std::size_t> getShader(std::string name) const = 0;
        
        /**
         * @brief Construct a new shader storage buffer
         *
         * @param name        The name of the shader storage buffer
         * @param binding_point The binding point of the shader storage buffer
         * @param size        The size of the shader storage buffer in bytes
         * @param data        The data to initialize the shader storage buffer with
         *
         * @return The id of the shader storage buffer, or std::nullopt if an error occurred
         */
        virtual asio::awaitable<std::optional<std::size_t>> createShaderStorageBuffer(std::string name, unsigned int binding_point, const std::size_t size, const void * data) = 0;

        /**
         * @brief Update a shader storage buffer
         *
         * @param id          The id of the shader storage buffer to update
         * @param size        The size of the shader storage buffer in bytes
         * @param data        The data to update the shader storage buffer with
         *
         * @return `true` if the shader storage buffer was successfully updated, `false` otherwise
         */
        virtual asio::awaitable<bool> updateShaderStorageBuffer(std::size_t id, const std::size_t size, const void * data) = 0;

        /**
         * @brief Erase a shader storage buffer
         *
         * @param id The id of the shader storage buffer to erase
         *
         * @return `true` if the shader storage buffer was successfully erased, `false` otherwise
         */
        virtual asio::awaitable<bool> eraseShaderStorageBuffer(std::size_t id) = 0;

        /**
         * @brief Get the id of a shader storage buffer by name
         *
         * @param name The name of the shader storage buffer
         *
         * @return The id of the shader storage buffer, or std::nullopt if not found
         */
        virtual std::optional<std::size_t> getShaderStorageBuffer(std::string name) const = 0;

        /**
         * @brief Construct a new Frame Buffer Object (FBO) object
         * 
         * This method is used to create a new Frame Buffer Object (FBO) which is a collection
         * of attachments (textures and/or render buffers) that can be used as a target for rendering.
         * The caller must provide a valid name for the FBO, a resolution, and a list of attachments.
         * 
         * @param name The name of the FBO.
         * @param resolution The resolution of the FBO.
         * @param attachments A list of attachments for the FBO.
         * @return An awaitable that resolves to the ID of the created FBO or an empty optional if the
         * FBO could not be created.
         */
        virtual asio::awaitable<std::optional<std::size_t>> createFrameBufferObject(std::string name, std::pair<unsigned int, unsigned int> resolution, std::initializer_list<FBOAttachment> attachments) = 0;
        
        /**
         * @brief Erase a Frame Buffer Object (FBO) object by ID
         * 
         * This method is used to erase a Frame Buffer Object (FBO) object by its ID.
         * 
         * @param id The ID of the FBO object to erase.
         * @return An awaitable that resolves to `true` if the FBO object was successfully erased, or `false` if it could not be erased.
         */
        virtual asio::awaitable<bool> eraseFrameBufferObject(std::size_t id) = 0;
        
        /**
         * @brief Get the ID of a Frame Buffer Object (FBO) object by name
         * 
         * This method is used to get the ID of a Frame Buffer Object (FBO) object by its name.
         * 
         * @param name The name of the FBO object to get the ID of.
         * @return An optional size_t containing the ID of the FBO object, or an empty optional if the FBO object could not be found.
         */
        virtual std::optional<std::size_t> getFrameBufferObject(std::string name) const = 0;
        
        /**
         * @brief Get the IDs of the textures attached to a Frame Buffer Object (FBO) object
         * 
         * This method is used to get the IDs of the textures attached to a Frame Buffer Object (FBO) object.
         * 
         * @param id The ID of the FBO object to get the texture IDs of.
         * @return A vector of size_t containing the IDs of the textures attached to the FBO object.
         */
        virtual std::vector<std::size_t> getFrameBufferObjectTextures(std::size_t id) const = 0;
    };

    template<class RendererImplType>
    class RendererModel final : public type::ModelBase<IRenderer, RendererImplType>
    {
        public:
            using base = type::ModelBase<IRenderer, RendererImplType>;

            template<class... Args>                                                                             
            inline RendererModel(Args && ... args) 
                : base(std::forward<Args>(args)...)
            {}

            inline bool good() const override { return base::impl().good();}
            inline asio::awaitable<void> close() override { return base::impl().close();}
            inline void join() override { return base::impl().join();}

            inline asio::awaitable<void> clearScreen(glm::vec4 color, std::optional<std::size_t> fbo) override { 
                return base::impl().clearScreen(std::move(color), std::move(fbo));
            }

            inline asio::awaitable<void> render(std::size_t vertex_buffer, std::size_t shader, 
                ShaderInputs shader_inputs,
                RenderOptions options,
                std::optional<std::size_t> fbo) override 
            { 
                return base::impl().render(std::move(vertex_buffer), std::move(shader),
                    std::move(shader_inputs),
                    std::move(options),
                    std::move(fbo)
                );
            }

            inline asio::awaitable<void> present() override { 
                return base::impl().present();
            }

            inline asio::awaitable<void> updateViewportSize(std::pair<unsigned int, unsigned int> resolution) override { 
                return base::impl().updateViewportSize(std::move(resolution));
            }

            inline std::pair<unsigned int, unsigned int> getViewportSize() const override { 
                return base::impl().getViewportSize();
            }

            inline asio::awaitable<void> enableVSync() override { 
                return base::impl().enableVSync();
            }

            inline asio::awaitable<void> disableVSync() override { 
                return base::impl().disableVSync();
            }

            inline asio::awaitable<std::optional<std::size_t>> createVertexBuffer(std::string name, const Mesh & mesh) override{ 
                return base::impl().createVertexBuffer(std::move(name), mesh);
            }
        
            inline asio::awaitable<bool> eraseVertexBuffer(std::size_t id) override {
                return base::impl().eraseVertexBuffer(std::move(id));
            }

            inline std::optional<std::size_t> getVertexBuffer(std::string name) const override{
                return base::impl().getVertexBuffer(std::move(name));
            }

            inline asio::awaitable<std::optional<std::size_t>> createShader(std::string name, std::vector<std::string> vertex_code) override { 
                return base::impl().createShader(std::move(name), std::move(vertex_code));
            }

            inline asio::awaitable<std::optional<std::size_t>> createShader(std::string name, std::vector<std::string> vertex_code, std::vector<std::string> fragment_code) override { 
                return base::impl().createShader(std::move(name), std::move(vertex_code), std::move(fragment_code));
            }

            inline asio::awaitable<bool> eraseShader(std::size_t id) override {
                return base::impl().eraseShader(std::move(id));
            }
            
            inline std::optional<std::size_t> getShader(std::string name) const override{
                return  base::impl().getShader(std::move(name));
            }

            inline asio::awaitable<std::optional<std::size_t>> createShaderStorageBuffer(std::string name, unsigned int binding_point, const std::size_t size, const void * data) override{ 
                return base::impl().createShaderStorageBuffer(std::move(name), binding_point, std::move(size), std::move(data));
            }

            inline asio::awaitable<bool> updateShaderStorageBuffer(std::size_t id, const std::size_t size, const void * data) override {
                return base::impl().updateShaderStorageBuffer(std::move(id), std::move(size), std::move(data));
            }

            inline asio::awaitable<bool> eraseShaderStorageBuffer(std::size_t id) override {
                return base::impl().eraseShaderStorageBuffer(std::move(id));
            }

            inline std::optional<std::size_t> getShaderStorageBuffer(std::string name) const override{
                return base::impl().getShaderStorageBuffer(std::move(name));
            }

            inline asio::awaitable<std::optional<std::size_t>> createFrameBufferObject(std::string name, std::pair<unsigned int, unsigned int> resolution, std::initializer_list<FBOAttachment> attachments) override{ 
                return base::impl().createFrameBufferObject(std::move(name), std::move(resolution), std::move(attachments));
            }

            inline asio::awaitable<bool> eraseFrameBufferObject(std::size_t id) override {
                return base::impl().eraseFrameBufferObject(std::move(id));
            }

            inline std::optional<std::size_t> getFrameBufferObject(std::string name) const override{
                return base::impl().getFrameBufferObject(std::move(name));
            }

            inline std::vector<std::size_t> getFrameBufferObjectTextures(std::size_t id) const override {
                return base::impl().getFrameBufferObjectTextures(std::move(id));
            }
    };

    template<class RendererImplType>
    RendererModel(RendererImplType && ) -> RendererModel<RendererImplType>;

    class Renderer final : public type::Implementation<IRenderer, RendererModel> 
    {                                                                   
        public:
            /* move ctor */
            inline Renderer(Renderer && other) = default;
            inline Renderer & operator=(Renderer && other) = default;
        
            /* ctor */
            template<class RendererImplType, typename... Args>
            inline Renderer(Args && ... args)
                : Implementation(std::in_place_type<RendererImplType>, std::forward<Args>(args)...)
            {}

            /* ctor */
            template<class RendererImplType, typename... Args>
            inline Renderer(std::in_place_type_t<RendererImplType>, Args && ... args)
                : Implementation(std::in_place_type<RendererImplType>, std::forward<Args>(args)...)
            {}

    };
}