#pragma once

#include <thread>

#include "formatter/math_format.hpp"
#include "formatter/native_format.hpp"
#include "formatter/opengl_format.hpp"

template <>
struct std::formatter<std::thread::id> : std::formatter<std::string> {
  auto format(std::thread::id id, format_context& ctx) const {
    return formatter<string>::format(
      std::format("window_handle[{}]", (std::stringstream{} << id).str()), ctx);
  }
};