#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "render/opengl/opengl.hpp"
#include "unit_tests.hpp"

using namespace astre::render::opengl;
using namespace astre::window;
using namespace testing;

namespace {

class MockWindow : public IWindow {
public:
    MOCK_METHOD(astre::native::device_context, acquireDeviceContext, (), (override));
    MOCK_METHOD(bool, releaseDeviceContext, (astre::native::device_context), (override));
    MOCK_METHOD(astre::process::IProcess&, getProcess, (), (override));
    MOCK_METHOD(unsigned int, getWidth, (), (const, override));
    MOCK_METHOD(unsigned int, getHeight, (), (const, override));
    MOCK_METHOD(astre::native::window_handle, getHandle, (), (const, override));
    MOCK_METHOD(bool, good, (), (const, override));
    MOCK_METHOD(asio::awaitable<void>, show, (), (override));
    MOCK_METHOD(asio::awaitable<void>, hide, (), (override));
    MOCK_METHOD(asio::awaitable<void>, close, (), (override));
    MOCK_METHOD(void, move, (astre::type::InterfaceBase *), (override));
    MOCK_METHOD(void, copy, (astre::type::InterfaceBase *), (const, override));
    MOCK_METHOD(std::unique_ptr<astre::type::InterfaceBase>, clone, (), (const, override));

};

astre::native::device_context dummy_dc = reinterpret_cast<astre::native::device_context>(0x1337);
astre::native::opengl_context_handle dummy_glctx = reinterpret_cast<astre::native::opengl_context_handle>(0xBEEF);

} // namespace

class RenderThreadContextTest : public ::testing::Test {
protected:
    NiceMock<MockWindow> mock_window;

    astre::native::device_context device = dummy_dc;
    astre::native::opengl_context_handle glctx = dummy_glctx;

    std::unique_ptr<OpenGLRenderThreadContext> context;

    void SetUp() override 
    {
        EXPECT_CALL(mock_window, acquireDeviceContext())
            .WillOnce(Return(dummy_dc));


        context = std::make_unique<OpenGLRenderThreadContext>(mock_window, glctx);
    }

    void TearDown() override {
        if (context) {
            context->join();
            context.reset();
        }
    }
};

TEST_F(RenderThreadContextTest, ThreadStartsAndJoinsCleanly) {
    ASSERT_TRUE(context->running());

    context->join();

    EXPECT_FALSE(context->running());
}

TEST_F(RenderThreadContextTest, SignalCloseStopsContext) {
    ASSERT_TRUE(context->running());

    context->signalClose();

    // Give some time for the thread to exit
    context->join();

    EXPECT_FALSE(context->running());
}


