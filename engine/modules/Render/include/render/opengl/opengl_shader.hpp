#pragma once

#include <string>
#include <optional>
#include <utility>

#include <GL/glew.h>

#include <absl/container/flat_hash_map.h>
#include <spdlog/spdlog.h>

#include "math/math.hpp"
#include "process/process.hpp"

#include "render/vertex.hpp"
#include "render/texture.hpp"

#include "render/opengl/opengl_debug.hpp"
#include "render/opengl/glsl_variable.hpp"

namespace astre::render::opengl
{
    class OpenGLShader
    {
    public:
        //ctor
        OpenGLShader(std::vector<std::string> vertex_code);
        OpenGLShader(std::vector<std::string> vertex_code, std::vector<std::string> fragment_code);
        OpenGLShader(OpenGLShader && other);
        ~OpenGLShader();

        std::size_t ID() const;

        bool enable();
        bool disable();

        void setUniform(const std::string & name, bool value);

        void setUniform(const std::string & name, int value);
        void setUniform(const std::string & name, std::uint32_t value);
        void setUniform(const std::string & name, float value);

        void setUniform(const std::string & name, const math::Vec2 & value);
        void setUniform(const std::string & name, const math::Vec3 & value);
        void setUniform(const std::string & name, const math::Vec4 & value);

        void setUniform(const std::string & name, const math::Mat2 & value);
        void setUniform(const std::string & name, const math::Mat3 & value);
        void setUniform(const std::string & name, const math::Mat4 & value);
        void setUniform(const std::string & name, const std::vector<math::Mat4> & values);

        void setUniform(const std::string & name, unsigned int unit, const ITexture & value);
        void setUniform(const std::string & name, unsigned int unit, const std::vector<ITexture*> & values);


    protected:
        OpenGLShader();

        class Stage
        {
            public:
                Stage(GLuint linked_shader_ID, GLenum stage_type, std::vector<std::string> code);
                Stage(Stage && other);
                Stage & operator=(Stage && other);
                Stage(const Stage & other) = delete;
                Stage & operator=(const Stage & other) = delete;
                ~Stage();

            private:
                static constexpr unsigned int _MAX_LOG_LENGTH = 1024;

                GLint _result = 0;
                GLenum _stage_type = GL_INVALID_ENUM;
                GLuint _stage_ID = 0;
                GLuint _linked_shader_ID = 0;
        };

        bool generateProgramID();

        bool linkProgram();
        bool validateProgram() const;

        void fetchAttributes();
        void fetchUniforms();

    private:
        static constexpr unsigned int _MAX_LOG_LENGTH = 4096;

        GLuint _shader_program_ID;

        std::optional<Stage> _vertex_stage;
        std::optional<Stage> _fragment_stage;
        
        absl::flat_hash_map<std::string, std::pair<GLint, GLSLVariable>> _uniforms;
        absl::flat_hash_map<std::string, std::pair<GLint, GLSLVariable>> _attributes;
    };
}