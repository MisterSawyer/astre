#pragma once

#include <string>
#include <format>
#include <sstream>

#include <GL/glew.h>


template <>
struct std::formatter<GLuint> : std::formatter<std::string> {
  auto format(GLuint ogl_uint, format_context& ctx) const {
    return formatter<string>::format(
      std::format("{}", (std::stringstream{} << ogl_uint).str()), ctx);
  }
};

template <>
struct std::formatter<const GLubyte *> : std::formatter<std::string> {
  auto format(const GLubyte * ogl_byte_str, format_context& ctx) const {
    return formatter<string>::format(
      std::format("{}", (std::stringstream{} << ogl_byte_str).str()), ctx);
  }
};

template <>
struct std::formatter<const GLchar *> : std::formatter<std::string> {
  auto format(const GLchar * ogl_char_str, format_context& ctx) const {
    return formatter<string>::format(
      std::format("{}", (std::stringstream{} << ogl_char_str).str()), ctx);
  }
};