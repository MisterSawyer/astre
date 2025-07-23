#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "render/opengl/opengl_texture.hpp"

using namespace astre::render;
using namespace astre::render::opengl;

// ==== Mocks / Stubs ====

namespace {
// Stubbed OpenGL state checker
std::expected<void, std::string> checkOpenGLState() {
    return {}; // always success
}

} // namespace

// ==== TESTS ====

TEST(OpenGLTextureTest, TextureFormatConversionIsCorrect) {
    EXPECT_EQ(textureFormatToOpenGLFormat(TextureFormat::RGB_8), GL_RGB8UI);
    EXPECT_EQ(textureFormatToOpenGLFormat(TextureFormat::RGBA_32F), GL_RGBA32F);
    EXPECT_EQ(textureFormatToOpenGLFormat(TextureFormat::Depth_32F), GL_DEPTH_COMPONENT32F);
}

TEST(OpenGLTextureTest, BaseFormatAndTypeAreResolvedCorrectly) {
    auto [format, type] = getBaseFormatAndType(GL_RGBA8UI);
    EXPECT_EQ(format, GL_RGBA_INTEGER);
    EXPECT_EQ(type, GL_UNSIGNED_BYTE);

    std::tie(format, type) = getBaseFormatAndType(GL_DEPTH_COMPONENT32F);
    EXPECT_EQ(format, GL_DEPTH_COMPONENT);
    EXPECT_EQ(type, GL_FLOAT);
}

