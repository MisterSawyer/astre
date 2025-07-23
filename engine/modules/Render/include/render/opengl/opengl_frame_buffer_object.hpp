#pragma once

#include <utility>
#include <cassert>

#include <spdlog/spdlog.h>
#include <absl/container/flat_hash_map.h>
#include <GL/glew.h>

#include "render/frame_buffer_object.hpp"
#include "render/texture.hpp"
#include "render/render_buffer.hpp"

#include "render/opengl/glsl_variable.hpp"
#include "render/opengl/opengl_texture.hpp"
#include "render/opengl/opengl_render_buffer_object.hpp"

namespace astre::render::opengl
{
    /**
     * @brief OpenGL Frame Buffer Object
     * 
     * This class is used to create and manage an OpenGL Frame Buffer Object (FBO).
     * It provides methods to enable, disable, and check the status of the FBO.
     * 
     * @note This class is not thread-safe. Must be used in renderer thread.
     */
    class OpenGLFrameBufferObject
    {
        public:
            /**
             * @brief Construct a new OpenGL Frame Buffer Object object
             * 
             * @param resolution The resolution of the frame buffer object.
             * @param attachments The attachments to be attached to the frame buffer object.
             * @note The attachments are a vector of pairs, where the first element is the constructed attachement ID 
             * and the second element is the attachment information.
             */
            OpenGLFrameBufferObject(std::pair<unsigned int, unsigned int> resolution, std::vector<std::pair<std::size_t, FBOAttachment>> attachments);
            
            ~OpenGLFrameBufferObject();

            OpenGLFrameBufferObject(OpenGLFrameBufferObject && other);

            /**
             * @brief Get the ID of the frame buffer object.
             * 
             * @return The ID of the frame buffer object.
             */
            std::size_t ID() const;

            /**
             * @brief Check if the frame buffer object is valid.
             * 
             * @return True if the frame buffer object is valid, otherwise false.
             */
            bool good() const;

            /**
             * @brief Enable the OpenGL Frame Buffer Object for rendering.
             * 
             * Bind the frame buffer object to the current OpenGL context for rendering.
             * 
             * @return True if the frame buffer object is valid and was successfully enabled, otherwise false.
             */
            bool enable() const;
            
            /**
             * @brief Disable the OpenGL Frame Buffer Object for rendering.
             * 
             * Unbind the frame buffer object from the current OpenGL context for rendering.
             * 
             * @return None
             */
            void disable() const;
            
            /**
             * @brief Get the textures attached to the frame buffer object.
             * 
             * @return The textures attached to the frame buffer object.
             */
            const std::vector<std::size_t> & getTextures() const;

            /**
             * @brief Get the resolution of the frame buffer object.
             * 
             * @return The resolution of the frame buffer object.
             */
            std::pair<unsigned int, unsigned int> getResolution() const;

        protected:
            
            bool generateBuffer();
            bool removeBuffer();
            void processAttachments(const std::vector<std::pair<std::size_t, FBOAttachment>> & attachments);

        private:
            GLuint _FBO;
            std::pair<unsigned int, unsigned int> _resolution;

            std::vector<std::size_t> _attached_textures;
            std::vector<std::size_t> _attached_render_buffers;
    };
}