#pragma once

#include <format>
#include <string>

#include "native/native.h"

template <>
struct std::formatter<astre::native::window_handle> : std::formatter<std::string> {
  auto format(astre::native::window_handle handle, format_context& ctx) const {
    return formatter<string>::format(
      std::format("window_handle[{}]", (std::stringstream{} << handle).str()), ctx);
  }
};

template <>
struct std::formatter<astre::native::opengl_context_handle> : std::formatter<std::string> {
  auto format(astre::native::opengl_context_handle handle, format_context& ctx) const {
    return formatter<string>::format(
      std::format("opengl_context[{}]", (std::stringstream{} << handle).str()), ctx);
  }
};

template <>
struct std::formatter<astre::native::device_context> : std::formatter<std::string> {
  auto format(astre::native::device_context dc, format_context& ctx) const {
    return formatter<string>::format(
      std::format("device_context[{}]", (std::stringstream{} << dc).str()), ctx);
  }
};