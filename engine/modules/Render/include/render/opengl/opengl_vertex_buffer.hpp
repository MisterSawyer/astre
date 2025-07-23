#pragma once

#include <utility>

#include <spdlog/spdlog.h>
#include <GL/glew.h>

#include "render/vertex.hpp"

#include "render/opengl/opengl_debug.hpp"

namespace astre::render::opengl
{
    class OpenGLVertexBuffer
    {
        public:
            OpenGLVertexBuffer(std::vector<unsigned int> indices, std::vector<Vertex> vertices);
            
            ~OpenGLVertexBuffer();

            OpenGLVertexBuffer(OpenGLVertexBuffer && other);

            std::size_t ID() const;

            bool good() const;

            std::size_t numberOfElements() const;
            //
            bool enable() const;
            //
            void disable() const;

        protected:
            bool setGPUAttributes();
            bool generateBuffers();
            bool removeBuffers();
            bool copyDataToGPU() const;

        private:
            GLuint _VAO;
            GLuint _VBO;
            GLuint _EBO;

            std::vector<GLuint> _indices;
            std::vector<Vertex> _vertices;
    };
}