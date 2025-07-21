#include <gtest/gtest.h>
#include "native/native.h"

using namespace astre::native;

TEST(RunProcessTest, InvalidCommandReturnsError) {

    EXPECT_THROW(runProcess("nonexistent_command"), std::runtime_error);
}

#if defined(WIN32)
TEST(RunProcessTest, RunsEchoCommandSuccessfully) {
    auto [exitCode, output] = runProcess("cmd", {"/C", "echo Hello, World!"});
    EXPECT_EQ(exitCode, 0);
    EXPECT_NE(output.find("Hello"), std::string::npos);
}
#endif

#if defined(WIN32)
TEST(RunProcessTest, ArgumentsAreHandledCorrectly) {

    auto [exitCode, output] = runProcess("cmd", {"/C", "echo", "test1", "test2"});
    EXPECT_EQ(exitCode, 0);
    EXPECT_NE(output.find("test1"), std::string::npos);
    EXPECT_NE(output.find("test2"), std::string::npos);
}
#endif
