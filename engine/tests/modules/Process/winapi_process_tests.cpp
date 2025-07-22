#include <gtest/gtest.h>
#include "unit_tests.hpp"
#include "process/windows/process_windows.hpp"

using namespace astre::tests;
using namespace astre::process;
using namespace astre::process::windows;


class WinapiProcessTest : public ::testing::Test {
protected:
    WinapiProcess process;

    void TearDown() override {
        // Make sure the process is fully closed after each test
        process.join();
    }
};

TEST_F(WinapiProcessTest, RegisterAndUnregisterWindow) {
    auto window = sync_await(process.registerWindow("TestWindow", 100, 100));
    ASSERT_NE(window, nullptr);

    auto unreg_result = sync_await(process.unregisterWindow(window));
    EXPECT_TRUE(unreg_result);
}


TEST_F(WinapiProcessTest, SetWindowCallbacksWorks) {
    auto window = sync_await(process.registerWindow("CallbackTest", 100, 100));
    ASSERT_NE(window, nullptr);

    WindowCallbacks callbacks;
    callbacks.executor = asio::system_executor(); // use dummy executor
    callbacks.onResize = [](int, int) -> asio::awaitable<void> { co_return; };

    auto result = sync_await(process.setWindowCallbacks(window, std::move(callbacks)));
    EXPECT_TRUE(result);

    auto unreg_result = sync_await(process.unregisterWindow(window));
    EXPECT_TRUE(unreg_result);
}

TEST_F(WinapiProcessTest, RegisterUnregisterOGLContext) {
    auto window = sync_await(process.registerWindow("OGLWindow", 100, 100));
    ASSERT_NE(window, nullptr);

    auto context = sync_await(process.registerOGLContext(window, 3, 3));
    ASSERT_NE(context, nullptr);

    auto result = sync_await(process.unregisterOGLContext(context));
    EXPECT_TRUE(result);

    auto unreg_result = sync_await(process.unregisterWindow(window));
    EXPECT_TRUE(unreg_result);
}