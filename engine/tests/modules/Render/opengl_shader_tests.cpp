#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unit_tests.hpp"
#include "process/process.hpp"
#include "window/window.hpp"
#include "render/opengl/opengl_shader.hpp"

using namespace astre;
using namespace astre::tests;
using namespace astre::render;
using namespace astre::render::opengl;

class OpenGLShaderRealContextTest : public ::testing::Test {
protected:
    asio::io_context io_ctx;

    process::Process process;
    window::Window window;
    native::device_context dc = nullptr;
    native::opengl_context_handle ogl_context = nullptr;

    OpenGLShaderRealContextTest()
        :   process(process::createProcess()),
            window(sync_await(window::createWindowAsync(io_ctx, *process, "Test Window", 640, 480)))
    {        
        dc = window->acquireDeviceContext();
        
        ogl_context = sync_await(process->registerOGLContext(window->getHandle(), 3, 3));
        EXPECT_NE(ogl_context, nullptr);

        wglMakeCurrent(dc, ogl_context);
    }

    void TearDown() override 
    {
        wglMakeCurrent(0, 0);

        if (ogl_context) {
            sync_await(process->unregisterOGLContext(ogl_context));
        }

        EXPECT_TRUE(window->releaseDeviceContext(dc));

        sync_await(io_ctx, window->close());

        process->join();
    }
};


// ==== TESTS ====

TEST_F(OpenGLShaderRealContextTest, CreatesShaderFromVertexCodeOnly) {
    std::vector<std::string> vertex_code = {
        "#version 330 core\n void main() {}"
    };

    OpenGLShader shader(vertex_code);
    EXPECT_GT(shader.ID(), 0);
}

TEST_F(OpenGLShaderRealContextTest, CreatesShaderFromVertexAndFragmentCode) {
    std::vector<std::string> vertex_code = {"#version 330 core\n void main() {}"};
    std::vector<std::string> fragment_code = {"#version 330 core\n void main() {}"};

    OpenGLShader shader(vertex_code, fragment_code);
    EXPECT_GT(shader.ID(), 0);
}

TEST_F(OpenGLShaderRealContextTest, MoveConstructsCorrectly) {
    std::vector<std::string> vertex_code = {"#version 330 core\n void main() {}"};
    OpenGLShader original(vertex_code);
    std::size_t id = original.ID();

    OpenGLShader moved = std::move(original);

    EXPECT_EQ(moved.ID(), id);
}

