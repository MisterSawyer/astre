#include <gtest/gtest.h>
#include "unit_tests.hpp"

#include "native/native.h"
#include "window/window.hpp"
#include "window/windows/window_windows.hpp"

#include "process/process.hpp"
#include "process/windows/process_windows.hpp"

using namespace astre::tests;
using namespace astre::native;
using namespace astre::window;
using namespace astre::window::windows;

class WinapiWindowTest : public ::testing::Test {
protected:
    WinapiWindowTest() : process(astre::process::createProcess()) {}

    asio::io_context io_ctx;
    astre::process::Process process;
    window_handle dummy_handle = reinterpret_cast<window_handle>(0x1234);

    void TearDown() override {
        // Make sure the process is fully closed after each test
        process->join();
    }
};

TEST_F(WinapiWindowTest, ConstructsCorrectlyAndAccessorsWork) {
    WinapiWindow window(io_ctx, *process, dummy_handle, 800, 600);

    EXPECT_EQ(window.getHandle(), dummy_handle);
    EXPECT_EQ(window.getWidth(), 800);
    EXPECT_EQ(window.getHeight(), 600);
    EXPECT_TRUE(window.good());

    sync_await(io_ctx, window.close());
}

TEST_F(WinapiWindowTest, MoveConstructorTransfersOwnership) {
    WinapiWindow original(io_ctx, *process, dummy_handle, 1024, 768);
    WinapiWindow moved(std::move(original));

    EXPECT_EQ(moved.getWidth(), 1024);
    EXPECT_EQ(moved.getHeight(), 768);
    EXPECT_EQ(moved.getHandle(), dummy_handle);

    sync_await(io_ctx, moved.close());
}

TEST_F(WinapiWindowTest, AcquireAndReleaseDeviceContext) 
{
    auto window_handle = sync_await(io_ctx, process->registerWindow("Test Window", 640, 480));
    EXPECT_NE(window_handle, nullptr);

    WinapiWindow window(io_ctx, *process, window_handle, 640, 480);

    auto dc = GetDC(window_handle);
    ReleaseDC(window_handle, dc);

    // Manually inject the context to simulate acquisition
    EXPECT_FALSE(window.releaseDeviceContext(dc));  // should be false, nothing to release yet
    EXPECT_EQ(window.acquireDeviceContext(), dc);
    EXPECT_TRUE(window.releaseDeviceContext(dc));

    sync_await(io_ctx, window.close());
}

TEST_F(WinapiWindowTest, TestCreationMethod) 
{
    Window window = sync_await(createWindowAsync(io_ctx, *process, "Test Window", 640, 480));
    EXPECT_TRUE(bool(window));
    EXPECT_NE(window->getHandle(), nullptr);
    EXPECT_EQ(window->getWidth(), 640);
    EXPECT_EQ(window->getHeight(), 480);
    EXPECT_TRUE(window->good());
    auto dc = window->acquireDeviceContext();
    EXPECT_NE(dc, nullptr);
    EXPECT_TRUE(window->releaseDeviceContext(dc));

    sync_await(io_ctx, window->close());
}
