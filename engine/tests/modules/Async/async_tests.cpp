#include <gtest/gtest.h>
#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>

#include "async/async.hpp"

using namespace std::chrono_literals;
using namespace astre::async;