#pragma once

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

namespace velora::tests
{
    class BaseUnitTest : public testing::Test
    {
        public:
            virtual ~BaseUnitTest() = default;

            static void SetUpTestSuite()
            {
                spdlog::set_level(spdlog::level::debug);
            }

            static void TearDownTestSuite()
            {  
            }
    };

    class UnitTest : public BaseUnitTest
    {
        public:
            UnitTest() = default;
    };
}