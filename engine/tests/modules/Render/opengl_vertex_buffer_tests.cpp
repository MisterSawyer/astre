#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "render/opengl/opengl_vertex_buffer.hpp"
#include "render/vertex.hpp"

using namespace astre::render::opengl;

namespace {
// Basic stub Vertex
astre::render::GPUVertex makeVertex(float x, float y, float z) {
    astre::render::GPUVertex v{};
    v.position = {x, y, z};
    v.normal = {0, 0, 1};
    v.uv = {0, 0};
    return v;
}

} // namespace

// ==== TESTS ====

TEST(OpenGLVertexBufferTest, ConstructorWithValidDataInitializesSuccessfully) {
    std::vector<astre::render::GPUVertex> vertices = {
        makeVertex(0.f, 0.f, 0.f),
        makeVertex(1.f, 0.f, 0.f),
        makeVertex(0.f, 1.f, 0.f),
    };
    std::vector<unsigned int> indices = {0, 1, 2};

    OpenGLVertexBuffer vbo(std::move(indices), std::move(vertices));

    EXPECT_EQ(vbo.numberOfElements(), 3);
}

TEST(OpenGLVertexBufferTest, ConstructorWithEmptyIndicesFails) {
    std::vector<astre::render::GPUVertex> vertices = {makeVertex(0, 0, 0)};
    std::vector<unsigned int> indices = {};

    OpenGLVertexBuffer vbo(std::move(indices), std::move(vertices));

    EXPECT_FALSE(vbo.good());
    EXPECT_EQ(vbo.numberOfElements(), 0);
}

TEST(OpenGLVertexBufferTest, MoveConstructorTransfersOwnership) {
    std::vector<astre::render::GPUVertex> vertices = {
        makeVertex(0, 0, 0), makeVertex(1, 1, 1)
    };
    std::vector<unsigned int> indices = {0, 1};

    OpenGLVertexBuffer original(std::move(indices), std::move(vertices));
    GLuint vao = original.ID();

    OpenGLVertexBuffer moved = std::move(original);

    EXPECT_EQ(moved.ID(), vao);
}

