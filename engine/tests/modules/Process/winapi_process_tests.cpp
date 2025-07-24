#include <gtest/gtest.h>
#include "unit_tests.hpp"
#include "async/async.hpp"
#include "process/windows/process_windows.hpp"

using namespace astre::tests;
using namespace astre::process;
using namespace astre::process::windows;


class WinapiProcessTest : public ::testing::Test {
protected:
    WinapiProcess process;
    
    WinapiProcessTest() : process(1) {}

    void TearDown() override {
        // Make sure the process is fully closed after each test
        process.join();
    }
};

TEST_F(WinapiProcessTest, RegisterAndUnregisterWindow) 
{
    auto window = sync_await(process.getExecutionContext(), process.registerWindow("TestWindow", 100, 100));
    ASSERT_NE(window, nullptr);

    auto unreg_result = sync_await(process.getExecutionContext(), process.unregisterWindow(window));
    EXPECT_TRUE(unreg_result);
}


TEST_F(WinapiProcessTest, SetWindowCallbacksWorks) {
    auto window = sync_await(process.getExecutionContext(), process.registerWindow("CallbackTest", 100, 100));
    ASSERT_NE(window, nullptr);

    WindowCallbacks callbacks{.context = astre::async::AsyncContext(process.getExecutionContext())};
    callbacks.onResize = [](int, int) -> asio::awaitable<void> { co_return; };

    auto result = sync_await(process.getExecutionContext(), process.setWindowCallbacks(window, std::move(callbacks)));
    EXPECT_TRUE(result);

    auto unreg_result = sync_await(process.getExecutionContext(), process.unregisterWindow(window));
    EXPECT_TRUE(unreg_result);
}

TEST_F(WinapiProcessTest, RegisterUnregisterOGLContext) {
    auto window = sync_await(process.getExecutionContext(), process.registerWindow("OGLWindow", 100, 100));
    ASSERT_NE(window, nullptr);

    auto context = sync_await(process.getExecutionContext(), process.registerOGLContext(window, 3, 3));
    ASSERT_NE(context, nullptr);

    auto result = sync_await(process.getExecutionContext(), process.unregisterOGLContext(context));
    EXPECT_TRUE(result);

    auto unreg_result = sync_await(process.getExecutionContext(), process.unregisterWindow(window));
    EXPECT_TRUE(unreg_result);
}