#pragma once

#include <utility>

#include <spdlog/spdlog.h>
#include <GL/glew.h>

#include "render/opengl/opengl_debug.hpp"

namespace astre::render::opengl
{
    /**
     * @brief This class represents an OpenGL Render Buffer Object (RBO), which is a buffer that stores rendered images.
     * 
     * @note This buffer is write only, and cannot be sampled by any shader.
     */
    class OpenGLRenderBufferObject
    {
        public:

            /**
             * @brief Constructs an OpenGL Render Buffer Object (RBO) with the specified resolution and format.
             * 
             * This constructor generates an OpenGL Render Buffer Object, binds it to the OpenGL context,
             * and allocates storage for it with the given resolution and format.
             * 
             * @param resolution The resolution of the render buffer.
             * @param format The internal format to be used for the render buffer storage.
             */
            OpenGLRenderBufferObject(std::pair<unsigned int, unsigned int> resolution, GLenum format);
            
            /**
             * @brief Default destructor for OpenGLRenderBufferObject.
             */
            ~OpenGLRenderBufferObject() = default;

            /**
             * @brief Move constructor for OpenGLRenderBufferObject.
             */
            OpenGLRenderBufferObject(OpenGLRenderBufferObject && other) = default;

            /**
             * @brief Get the OpenGL ID of the render buffer object.
             * 
             * @return The OpenGL ID of the render buffer object.
             */
            std::size_t ID() const;

            /**
             * @brief Check if the OpenGL Render Buffer Object is valid.
             * 
             * @return True if the render buffer object has a valid ID, otherwise false.
             */

            bool good() const;

            /**
             * @brief Enable the OpenGL Render Buffer Object for rendering.
             * 
             * Bind the render buffer object to the current OpenGL context for rendering.
             * 
             * @return True if the render buffer object is valid and was successfully enabled, otherwise false.
             */
            bool enable() const;
            
            /**
             * @brief Disable the OpenGL Render Buffer Object for rendering.
             * 
             * Unbind the render buffer object from the current OpenGL context for rendering.
             * 
             * @return None
             */
            void disable() const;

        private:
            GLuint _ID;
            std::pair<unsigned int, unsigned int> _resolution;
    };
}