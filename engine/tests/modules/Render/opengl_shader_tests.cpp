#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "render/opengl/opengl_shader.hpp"

#include "unit_tests.hpp"
#include "process/process.hpp"
#include "window/window.hpp"

using namespace astre;
using namespace astre::tests;
using namespace astre::render;
using namespace astre::render::opengl;

class OpenGLShaderRealContextTest : public ::testing::Test {
protected:
    process::Process process;
    window::Window window;
    native::device_context dc = nullptr;
    native::opengl_context_handle ogl_context = nullptr;

    OpenGLShaderRealContextTest()
        :   process(process::createProcess(1)),
            window(sync_await(process->getExecutionContext(), window::createWindow( *process, "Test Window", 640, 480)))
    {        
        dc = window->acquireDeviceContext();
        
        ogl_context = sync_await(process->getExecutionContext(), process->registerOGLContext(window->getHandle(), 3, 3));
        EXPECT_NE(ogl_context, nullptr);

        #ifdef WIN32
            wglMakeCurrent(dc, ogl_context);
        #endif
    }

    void TearDown() override 
    {
        #ifdef WIN32
            wglMakeCurrent(0, 0);
        #endif

        if (ogl_context) {
            sync_await(process->getExecutionContext(), process->unregisterOGLContext(ogl_context));
        }

        EXPECT_TRUE(window->releaseDeviceContext(dc));

        sync_await(process->getExecutionContext(), window->close());

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

