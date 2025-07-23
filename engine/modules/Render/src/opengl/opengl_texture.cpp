#include "render/opengl/opengl_texture.hpp"

namespace astre::render::opengl
{
    GLenum textureFormatToOpenGLFormat(TextureFormat format)
    {
        switch(format)
        {
            case TextureFormat::RGB_8 :
                return GL_RGB8UI;
            case TextureFormat::RGB_16 :
                return GL_RGB16UI;
            case TextureFormat::RGB_32 :
                return GL_RGB32UI;

            case TextureFormat::RGB_16F :
                return GL_RGB16F;
            case TextureFormat::RGB_32F :
                return GL_RGB32F;

            case TextureFormat::RGBA_8 :
                return GL_RGBA8UI;
            case TextureFormat::RGBA_16 :
                return GL_RGBA16UI;
            case TextureFormat::RGBA_32 :
                return GL_RGBA32UI;

            case TextureFormat::RGBA_16F :
                return GL_RGBA16F;
            case TextureFormat::RGBA_32F :
                return GL_RGBA32F;

            case TextureFormat::Stencil :
                return GL_STENCIL_INDEX8;

            case TextureFormat::Depth :
                return GL_DEPTH_COMPONENT;

            case TextureFormat::Depth_32F :
                return GL_DEPTH_COMPONENT32F;
            
            default :
                spdlog::warn(std::format("[opengl] Unknown texture format "));
                return 0;
        }
    }

    std::pair<GLenum, GLenum> getBaseFormatAndType(GLenum format)
    {
        switch(format)
        {
            // RED formats
            case GL_R8:
            case GL_R8_SNORM:
                return {GL_RED, GL_BYTE};
            case GL_R16:
            case GL_R16_SNORM:
                return {GL_RED, GL_SHORT};
            case GL_R16F:
                return {GL_RED, GL_HALF_FLOAT};
            case GL_R32F:
                return {GL_RED, GL_FLOAT};
            case GL_R8I:
                return {GL_RED_INTEGER, GL_BYTE};
            case GL_R8UI:
                return {GL_RED_INTEGER, GL_UNSIGNED_BYTE};
            case GL_R16I:
                return {GL_RED_INTEGER, GL_SHORT};
            case GL_R16UI:
                return {GL_RED_INTEGER, GL_UNSIGNED_SHORT};
            case GL_R32I:
                return {GL_RED_INTEGER, GL_INT};
            case GL_R32UI:
                return {GL_RED_INTEGER, GL_UNSIGNED_INT};

            // RG formats
            case GL_RG8:
            case GL_RG8_SNORM:
                return {GL_RG, GL_BYTE};
            case GL_RG16:
            case GL_RG16_SNORM:
                return {GL_RG, GL_SHORT};
            case GL_RG16F:
                return {GL_RG, GL_HALF_FLOAT};
            case GL_RG32F:
                return {GL_RG, GL_FLOAT};
            case GL_RG8I:
                return {GL_RG_INTEGER, GL_BYTE};
            case GL_RG8UI:
                return {GL_RG_INTEGER, GL_UNSIGNED_BYTE};
            case GL_RG16I:
                return {GL_RG_INTEGER, GL_SHORT};
            case GL_RG16UI:
                return {GL_RG_INTEGER, GL_UNSIGNED_SHORT};
            case GL_RG32I:
                return {GL_RG_INTEGER, GL_INT};
            case GL_RG32UI:
                return {GL_RG_INTEGER, GL_UNSIGNED_INT};

            // RGB formats
            case GL_RGB8:
            case GL_RGB8_SNORM:
                return {GL_RGB, GL_BYTE};
            case GL_RGB16:
            case GL_RGB16_SNORM:
                return {GL_RGB, GL_SHORT};
            case GL_RGB16F:
                return {GL_RGB, GL_HALF_FLOAT};
            case GL_RGB32F:
                return {GL_RGB, GL_FLOAT};
            case GL_R3_G3_B2:
                return {GL_RGB, GL_UNSIGNED_BYTE_3_3_2};
            case GL_RGB4:
            case GL_RGB5:
            case GL_RGB10:
            case GL_RGB12:
            case GL_RGB9_E5:
            case GL_RGB10_A2UI:
                return {GL_RGB_INTEGER, GL_UNSIGNED_INT};
            case GL_RGB8I:
                return {GL_RGB_INTEGER, GL_BYTE};
            case GL_RGB8UI:
                return {GL_RGB_INTEGER, GL_UNSIGNED_BYTE};
            case GL_RGB16I:
                return {GL_RGB_INTEGER, GL_SHORT};
            case GL_RGB16UI:
                return {GL_RGB_INTEGER, GL_UNSIGNED_SHORT};
            case GL_RGB32I:
                return {GL_RGB_INTEGER, GL_INT};
            case GL_RGB32UI:
                return {GL_RGB_INTEGER, GL_UNSIGNED_INT};

            // RGBA formats
            case GL_RGBA8:
            case GL_RGBA8_SNORM:
                return {GL_RGBA, GL_BYTE};
            case GL_RGBA16:
            case GL_RGBA16_SNORM:
                return {GL_RGBA, GL_SHORT};
            case GL_RGBA16F:
                return {GL_RGBA, GL_HALF_FLOAT};
            case GL_RGBA32F:
                return {GL_RGBA, GL_FLOAT};
            case GL_RGBA2:
            case GL_RGBA4:
            case GL_RGB5_A1:
            case GL_RGB10_A2:
            case GL_RGBA12:
                return {GL_RGBA, GL_UNSIGNED_INT};
            case GL_RGBA8I:
                return {GL_RGBA_INTEGER, GL_BYTE};
            case GL_RGBA8UI:
                return {GL_RGBA_INTEGER, GL_UNSIGNED_BYTE};
            case GL_RGBA16I:
                return {GL_RGBA_INTEGER, GL_SHORT};
            case GL_RGBA16UI:
                return {GL_RGBA_INTEGER, GL_UNSIGNED_SHORT};
            case GL_RGBA32I:
                return {GL_RGBA_INTEGER, GL_INT};
            case GL_RGBA32UI:
                return {GL_RGBA_INTEGER, GL_UNSIGNED_INT};

            // DEPTH formats
            case GL_DEPTH_COMPONENT:
                return {GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE};
            case GL_DEPTH_COMPONENT16:
                return {GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
            case GL_DEPTH_COMPONENT24:
                return {GL_DEPTH_COMPONENT, GL_UNSIGNED_INT};
            case GL_DEPTH_COMPONENT32:
                return {GL_DEPTH_COMPONENT, GL_UNSIGNED_INT};
            case GL_DEPTH_COMPONENT32F:
                return {GL_DEPTH_COMPONENT, GL_FLOAT};

            // DEPTH + STENCIL formats
            case GL_DEPTH24_STENCIL8:
                return {GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8};
            case GL_DEPTH32F_STENCIL8:
                return {GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV};

            default:
                spdlog::warn(std::format("[opengl] Unsupported texture format 0x{:X}", format));
                return {format, 0}; // invalid fallback
        }
    }



    OpenGLTexture::OpenGLTexture(std::pair<unsigned int, unsigned int> resolution, GLenum format)
    :   _ID(0), _resolution(std::move(resolution))
    {
        glGenTextures(1, &_ID);
        auto check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] Texture glGenTextures failed, OpenGL error : {}", check.error());
        }

        enable();
        const auto [base_format, data_type] = getBaseFormatAndType(format);
        glTexImage2D(GL_TEXTURE_2D, 0, format, _resolution.first, _resolution.second, 0, base_format, data_type, nullptr);
        check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] Texture glTexImage2D failed, OpenGL error : {}", check.error());
        }
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	    //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	    //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] Texture glTexParameter failed, OpenGL error : {}", check.error());
        }

        disable();

        check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] Texture generation failed, OpenGL error : {}", check.error());
        }

    }

    std::size_t OpenGLTexture::ID() const
    {
        return _ID;
    }


    bool OpenGLTexture::good() const
    {
        return _ID != 0;
    }

    bool OpenGLTexture::enable() const
    {
        glBindTexture(GL_TEXTURE_2D, _ID);

        return true;
    }

    void OpenGLTexture::disable() const
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}