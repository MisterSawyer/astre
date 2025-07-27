#include "render/opengl/opengl_shader.hpp"

namespace astre::render::opengl
{
    OpenGLShader::Stage::Stage(GLuint linked_shader_ID, GLenum stage_type, std::vector<std::string> code)
    :   _linked_shader_ID(linked_shader_ID),
        _stage_type(stage_type),
        _stage_ID(0)
    {
        if(_linked_shader_ID == 0)
        {
            spdlog::error("Cannot build OpenGL shader stage - linked shader not specified");
            return;
        }

        if(_stage_type == GL_INVALID_ENUM)
        {
            spdlog::error("opengl-shader", "Cannot build OpenGL shader stage - stage type not specified.");
            return;
        }

        if(code.empty())
        {
            spdlog::error("opengl-shader", "Cannot build OpenGL shader stage - empty source code");
            return;
        }
        
        // allocate new stage ID
        _stage_ID = glCreateShader(_stage_type);

        std::vector<int> lengths;
        lengths.reserve(code.size());
        for (const auto &c : code)
        {
            lengths.push_back((int)c.length());
        }

        // convert to c-style string
        // need to make sure the strings are not destroyed before the shader is compiled
        // this array only holds the pointers to the strings, not the strings themselves
        std::vector<const char *> code_ptrs;
        code_ptrs.reserve(code.size());
        for (const auto &c : code)
        {
            code_ptrs.push_back(c.c_str());
        }

        glShaderSource(_stage_ID, (GLsizei)code_ptrs.size(), code_ptrs.data(), lengths.data());
        glCompileShader(_stage_ID);
        glGetShaderiv(_stage_ID, GL_COMPILE_STATUS, &_result);
        if (_result == GL_FALSE)
        {
            GLchar errors_log[_MAX_LOG_LENGTH];
            glGetShaderInfoLog(_stage_ID, _MAX_LOG_LENGTH, nullptr, errors_log);
            spdlog::error(std::format("OpenGL Shader stage[{}] compilation failed.\n{}", _stage_ID, errors_log));
            return;
        }
        spdlog::debug(std::format("OpenGL shader stage[{}] compiled successfully", _stage_ID));
        glAttachShader(_linked_shader_ID, _stage_ID);
    }

    OpenGLShader::Stage::Stage(Stage && other)
    :   _result(std::move(other._result)),
        _stage_type(std::move(other._stage_type)),
        _stage_ID(std::move(other._stage_ID)),
        _linked_shader_ID(std::move(other._linked_shader_ID))
    {
        other._result = 0; 
        other._stage_ID = 0;
        other._linked_shader_ID = 0;
        other._stage_type = GL_INVALID_ENUM;
    }

    OpenGLShader::Stage & OpenGLShader::Stage::operator=(Stage && other)
    {
        if(this == &other){return *this;}
        _result = std::move(other._result);
        _stage_type = std::move(other._stage_type);
        _stage_ID = std::move(other._stage_ID);
        _linked_shader_ID = std::move(other._linked_shader_ID);

        other._result = 0; 
        other._stage_ID = 0;
        other._linked_shader_ID = 0;
        other._stage_type = GL_INVALID_ENUM;

        return *this;
    }

    OpenGLShader::Stage::~Stage()
    {
        if(_stage_ID == 0){return;}
        if(_linked_shader_ID != 0)
        {
            spdlog::debug(std::format("Detaching OpenGL shader stage[{}] from program[{}]", _stage_ID, _linked_shader_ID));
            glDetachShader(_linked_shader_ID, _stage_ID);
            _linked_shader_ID = 0;
        }
        spdlog::debug(std::format("Deleting OpenGL shader stage[{}]", _stage_ID));
        glDeleteShader(_stage_ID);
        _stage_ID = 0; 
    }



    OpenGLShader::OpenGLShader()
    :   _shader_program_ID{0}
    {
        if(generateProgramID() == false)
        {
            spdlog::error("OpenGL shader program ID generation failed");
            return;
        }
        spdlog::info(std::format("OpenGL shader program ID {} generated", _shader_program_ID));
        
        const auto check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] Shader construction failed, OpenGL error : {}", check.error());
        }    
    }

    OpenGLShader::OpenGLShader(std::vector<std::string> vertex_code)
    :   OpenGLShader()
    {
        if(_shader_program_ID == 0)
        {
            spdlog::error("OpenGL shader program ID generation failed");
            return;
        }

        spdlog::debug(std::format("Compiling OpenGL shader[{}] : [Vertex Stage] ", _shader_program_ID));
        
        _vertex_stage = Stage(_shader_program_ID, GL_VERTEX_SHADER, std::move(vertex_code));
        
        if(linkProgram() == false)
        {
            spdlog::error(std::format("OpenGL shader[{}] linking failed", _shader_program_ID));
            return;
        }

        if(validateProgram() == false)
        {
            spdlog::error(std::format( "OpenGL shader[{}] did not pass validation", _shader_program_ID));
            return; 
        }

        fetchAttributes();
        fetchUniforms();
    }

    OpenGLShader::OpenGLShader(std::vector<std::string> vertex_code, std::vector<std::string> fragment_code)
    :   OpenGLShader()
    {
        if(_shader_program_ID == 0)
        {
            spdlog::error("OpenGL shader program ID generation failed");
            return;
        }

        spdlog::debug(std::format("Compiling OpenGL shader[{}] : [Vertex Stage] ", _shader_program_ID));
        
        _vertex_stage = Stage(_shader_program_ID, GL_VERTEX_SHADER, std::move(vertex_code));
        _fragment_stage = Stage(_shader_program_ID, GL_FRAGMENT_SHADER, std::move(fragment_code));
        
        if(linkProgram() == false)
        {
            spdlog::error(std::format("OpenGL shader[{}] linking failed", _shader_program_ID));
            return;
        }

        if(validateProgram() == false)
        {
            spdlog::error(std::format( "OpenGL shader[{}] did not pass validation", _shader_program_ID));
            return; 
        }

        fetchAttributes();
        fetchUniforms();
    }

    OpenGLShader::OpenGLShader(OpenGLShader && other)
    :   _shader_program_ID{std::move(other._shader_program_ID)},
        _vertex_stage{std::move(other._vertex_stage)},
        _fragment_stage{std::move(other._fragment_stage)},
        _uniforms{std::move(other._uniforms)},
        _attributes{std::move(other._attributes)}
    {
        other._shader_program_ID = 0;
    }

    OpenGLShader::~OpenGLShader()
    {
        if(_shader_program_ID == 0)return;

        disable();

        // clearing stages data
        if(_vertex_stage.has_value())_vertex_stage.reset();
        if(_fragment_stage.has_value())_fragment_stage.reset();

        glDeleteProgram(_shader_program_ID);
        spdlog::info(std::format("OpenGL shader[{}] destroyed", _shader_program_ID));

        _shader_program_ID = 0;
    }

    bool OpenGLShader::generateProgramID()
    {
        try{
            _shader_program_ID = glCreateProgram();
        }
        catch(...)
        {
            spdlog::error("Cannot create shader program - glCreateProgram error");
            return false;
        }

        const auto check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] Shader generate program id failed, OpenGL error : {}", check.error());
        }

        if(_shader_program_ID == 0)
        { 
            spdlog::error("Cannot create shader program - glCreateProgram error");
            return false;
        }
        return true;
    }

    std::size_t OpenGLShader::ID() const
    {
        return _shader_program_ID;
    }

    bool OpenGLShader::enable()
    {
        // already binded
        GLint currently_used_shader = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currently_used_shader);
        if(currently_used_shader == (GLint)_shader_program_ID){
            return true;
        }
        
        glUseProgram(_shader_program_ID);

        glGetIntegerv(GL_CURRENT_PROGRAM, &currently_used_shader);
        if(currently_used_shader != (GLint)_shader_program_ID){
            spdlog::error(std::format("Cannot bind OpenGL shader[{}], currently binded {}", _shader_program_ID, currently_used_shader));
            return false;
        }
        return true;
    }

    bool OpenGLShader::disable()
    {
        // only unbind if this is currently used
        GLint currently_used_shader = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currently_used_shader);
        if(currently_used_shader != (GLint)_shader_program_ID)
        {
            spdlog::debug(std::format("OpenGL shader[{}] is not currently used by opengl rendering state", _shader_program_ID));
            return false;
        }
        glUseProgram(0);
        return true;
    }

    bool OpenGLShader::linkProgram()
    {
        GLint result = GL_FALSE;
		glLinkProgram(_shader_program_ID);
        
        const auto check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] Shader linking program failed, OpenGL error : {}", check.error());
        }

		glGetProgramiv(_shader_program_ID, GL_LINK_STATUS, &result);
		if (result == GL_FALSE) // FAILS
        {
		    spdlog::error(std::format("OpenGL shader[{}] linking failed", _shader_program_ID));


            GLchar errors_log[_MAX_LOG_LENGTH];
		    glGetProgramInfoLog(_shader_program_ID, _MAX_LOG_LENGTH, nullptr, errors_log);
            spdlog::error(errors_log);
            return false;
        }

        return true;
    }

    bool OpenGLShader::validateProgram() const
    {
        GLint result = GL_FALSE;
        glValidateProgram(_shader_program_ID);
        
        const auto check = checkOpenGLState();
        if(!check)
        {
            spdlog::error("[opengl] Shader validating program failed, OpenGL error : {}", check.error());
        }

        glGetProgramiv(_shader_program_ID, GL_VALIDATE_STATUS, &result);
        if(result == GL_FALSE)
        {
            spdlog::error(std::format("OpenGL shader[{}] did not pass validation", _shader_program_ID));

            GLchar errors_log[_MAX_LOG_LENGTH];
            glGetProgramInfoLog(_shader_program_ID, _MAX_LOG_LENGTH, nullptr, errors_log);
            spdlog::error(errors_log);
            return false;
        }
        
        return true;
    }

    void OpenGLShader::fetchAttributes()
    {
        GLint attributes_count = 0;
        glGetProgramiv(_shader_program_ID, GL_ACTIVE_ATTRIBUTES, &attributes_count);

        for (GLint i = 0; i < attributes_count; ++i)
        {
            GLSLVariable var;
            glGetActiveAttrib(
                _shader_program_ID,         // program
                i,                          // uniform index
                GLSLVariable::buffer_size,  // max length of name buffer
                &var.length,                // actual name length
                &var.size,                  // array size (e.g. [4] is size 4)
                &var.type,                  // type: GL_FLOAT, GL_FLOAT_VEC3, etc.
                var.name                    // output name
            );
            GLint location = glGetAttribLocation(_shader_program_ID, var.name);

            _attributes[var.name] = std::make_pair(std::move(location), std::move(var));

            spdlog::debug("Attribute [{}]: name={}, type=0x{:X}, size={}", i, var.name, var.type, var.size);
        }
    }
    
    void OpenGLShader::fetchUniforms()
    {
        GLint uniform_count = 0;
        glGetProgramiv(_shader_program_ID, GL_ACTIVE_UNIFORMS, &uniform_count);

        for (GLint i = 0; i < uniform_count; ++i)
        {
            GLSLVariable var;
            glGetActiveUniform(
                _shader_program_ID,              // program
                i,                       // uniform index
                GLSLVariable::buffer_size,  // max length of name buffer
                &var.length,             // actual name length
                &var.size,               // array size (e.g. [4] is size 4)
                &var.type,               // type: GL_FLOAT, GL_FLOAT_VEC3, etc.
                var.name                 // output name
            );
            GLint location = glGetUniformLocation(_shader_program_ID, var.name);

            _uniforms[var.name] = std::make_pair(std::move(location), std::move(var));

            spdlog::debug("Uniform [{}]: name={}, type=0x{:X}, size={}", i, var.name, var.type, var.size);
        }
    }

    void OpenGLShader::setUniform(const std::string & name, bool value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniform1i(_uniforms.at(name).first, value);
    }

    void OpenGLShader::setUniform(const std::string & name, int value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniform1i(_uniforms.at(name).first, value);
    }

    void OpenGLShader::setUniform(const std::string & name, std::uint32_t value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniform1ui(_uniforms.at(name).first, value);
    }

    void OpenGLShader::setUniform(const std::string & name, float value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniform1f(_uniforms.at(name).first, value);
    }

    void OpenGLShader::setUniform(const std::string & name, const math::Vec2 & value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniform2fv(_uniforms.at(name).first, 1, math::value_ptr(value));
    }

    void OpenGLShader::setUniform(const std::string & name, const math::Vec3 & value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniform3fv(_uniforms.at(name).first, 1, math::value_ptr(value));
    }

    void OpenGLShader::setUniform(const std::string & name, const math::Vec4 & value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniform4fv(_uniforms.at(name).first, 1, math::value_ptr(value));
    }

    void OpenGLShader::setUniform(const std::string & name, const math::Mat2 & value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniformMatrix2fv(_uniforms.at(name).first, 1, GL_FALSE, math::value_ptr(value));
    }
        
    void OpenGLShader::setUniform(const std::string & name, const math::Mat3 & value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniformMatrix3fv(_uniforms.at(name).first, 1, GL_FALSE, math::value_ptr(value));
    }

    void OpenGLShader::setUniform(const std::string & name, const math::Mat4 & value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }
        glUniformMatrix4fv(_uniforms.at(name).first, 1, GL_FALSE, math::value_ptr(value));
    }

    void OpenGLShader::setUniform(const std::string & name, const std::vector<math::Mat4> & values)
    {
        if(values.empty())return;

        const std::string array_name = name + "[0]";
        if(_uniforms.contains(array_name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }

        const auto & glsl_var = _uniforms.at(array_name).second;
        if(values.size() > glsl_var.size)
        {
            spdlog::error("Uniform {} size {} is greater than the shader uniform array size {}", name, values.size(), glsl_var.size);
            return;
        }

        glUniformMatrix4fv(_uniforms.at(array_name).first, values.size(), GL_FALSE, math::value_ptr(values[0]));
    }

    void OpenGLShader::setUniform(const std::string & name, unsigned int unit, const ITexture & value)
    {
        if(_uniforms.contains(name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }

        glActiveTexture(GL_TEXTURE0 + unit);

        value.enable();

        glUniform1i(_uniforms.at(name).first, unit);
    }

    void OpenGLShader::setUniform(const std::string & name, unsigned int unit, const std::vector<ITexture*> & values)
    {
        if(values.empty())return;

        const std::string array_name = name + "[0]";

        if(_uniforms.contains(array_name) == false)
        {
            spdlog::error("Uniform {} not found in shader program {}", name, _shader_program_ID);
            return;
        }

        const auto & glsl_var = _uniforms.at(array_name).second;
        if(values.size() > glsl_var.size)
        {
            spdlog::error("Uniform {} size {} is greater than the shader uniform array size {}", name, values.size(), glsl_var.size);
            return;
        }

        std::vector<int> texture_units(values.size());
        for (unsigned int i = 0; i < values.size(); ++i)
        {
            texture_units[i] = unit + i;
            glActiveTexture(GL_TEXTURE0 + texture_units.at(i));
            values.at(i)->enable();
        }

        GLint base_location = glGetUniformLocation(_shader_program_ID, name.c_str());
        if (base_location == -1) {
            spdlog::error("Uniform base {} not found in shader program {}", name, _shader_program_ID);
            return;
        }

        glUniform1iv(base_location, static_cast<GLsizei>(values.size()), texture_units.data());
    }

}