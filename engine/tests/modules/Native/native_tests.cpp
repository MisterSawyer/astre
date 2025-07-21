#include <gtest/gtest.h>
#include "native/native.h"

using namespace astre::native;

TEST(RunProcessTest, RunsEchoCommandSuccessfully) {
#if defined(WIN32)
    auto [exitCode, output] = runProcess("cmd", {"/C", "echo Hello, World!"});
#else
    auto [exitCode, output] = runProcess("echo", {"Hello,", "World!"});
#endif

    EXPECT_EQ(exitCode, 0);
    EXPECT_NE(output.find("Hello"), std::string::npos);
}

TEST(RunProcessTest, InvalidCommandReturnsError) {

    EXPECT_THROW(runProcess("nonexistent_command"), std::runtime_error);
}

TEST(RunProcessTest, ArgumentsAreHandledCorrectly) {
#if defined(WIN32)
    auto [exitCode, output] = runProcess("cmd", {"/C", "echo", "test1", "test2"});
#else
    auto [exitCode, output] = runProcess("echo", {"test1", "test2"});
#endif

    EXPECT_EQ(exitCode, 0);
    EXPECT_NE(output.find("test1"), std::string::npos);
    EXPECT_NE(output.find("test2"), std::string::npos);
}
