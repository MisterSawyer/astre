#include "unit_tests.hpp"

int main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::debug);

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}