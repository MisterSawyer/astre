
#pragma once

#include <string>
#include <utility>

#include <glm/glm.hpp>
#include <absl/container/flat_hash_map.h>

#include "type/type.hpp"

#include "render/texture.hpp"

namespace astre::render
{
    /**
     * @brief Shader Inputs
     * 
     */
    struct ShaderInputs
    {
        absl::flat_hash_map<std::string, bool> in_bool;

        absl::flat_hash_map<std::string, int> in_int;
        absl::flat_hash_map<std::string, float> in_float;

        absl::flat_hash_map<std::string, glm::vec2> in_vec2;
        absl::flat_hash_map<std::string, glm::vec3> in_vec3;
        absl::flat_hash_map<std::string, glm::vec4> in_vec4;

        absl::flat_hash_map<std::string, glm::mat2> in_mat2;
        absl::flat_hash_map<std::string, glm::mat3> in_mat3;
        absl::flat_hash_map<std::string, glm::mat4> in_mat4;
        absl::flat_hash_map<std::string, std::vector<glm::mat4>> in_mat4_array;

        absl::flat_hash_map<std::string, std::size_t> in_samplers;
        absl::flat_hash_map<std::string, std::vector<std::size_t>> in_samplers_array;

        std::vector<std::size_t> storage_buffers;
    };

    /**
     * @brief Shader Interface
     * 
     */
    class IShader : public type::InterfaceBase
    {
        public:
        virtual ~IShader() {};

        virtual void move(IShader * dest) = 0;

        /**
         * @brief Get the ID of the shader.
         * 
         * @return The ID of the shader.
         */
        virtual std::size_t ID() const = 0;

        /**
         * @brief Check if the shader is valid.
         * 
         * @return True if the shader is valid, false otherwise.
         */
        virtual bool enable() = 0;

        /**
         * @brief Disable the shader.
         * 
         */
        virtual bool disable() = 0;

        /**
         * @brief Set boolean uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value Boolean value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, bool value) = 0;

        /**
         * @brief Set the integer uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value Integer value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, int value) = 0;

        /**
         * @brief Set the float uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value Float value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, float value)= 0;

        /**
         * @brief Set the 2D float vector uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value 2D float Vector value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, const glm::vec2 & value)= 0;

        /**
         * @brief Set the 3D float vector uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value 3D float Vector value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, const glm::vec3 & value)= 0;

        /**
         * @brief Set the 4D float vector uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value 4D float Vector value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, const glm::vec4 & value)= 0;

        /**
         * @brief Set the 2D float matrix uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value 2D float Matrix value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, const glm::mat2 & value)= 0;

        /**
         * @brief Set the 3D float matrix uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value 3D float Matrix value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, const glm::mat3 & value)= 0;

        /**
         * @brief Set the 4D float matrix uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value 4D float Matrix value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, const glm::mat4 & value)= 0;

        /**
         * @brief Set the 4D float matrix array uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value 4D float Matrix array value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, const std::vector<glm::mat4> & values)= 0;

        /**
         * @brief Set the texture uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value Texture value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, unsigned int unit, const ITexture & value)= 0;

        /**
         * @brief Set the texture array uniform value in the shader.
         * 
         * @param name The name of the uniform.
         * @param value Texture array value of the uniform.
         * 
         */
        virtual void setUniform(const std::string & name, unsigned int unit, const std::vector<ITexture*> & values)= 0;

    };

    template<class ShaderImplType>
    class ShaderModel final : public type::ModelBase<IShader, ShaderImplType>
    {
        public:
            using base = type::ModelBase<IShader, ShaderImplType>;

            template<class... Args>                                                                             
            inline ShaderModel(Args && ... args) 
                : base(std::forward<Args>(args)...)
            {} 

            explicit inline ShaderModel(ShaderImplType && obj)  
                : base(std::move(obj))
            {}


            inline void move(IShader * dest) override
            {
                ::new(dest) ShaderModel(std::move(base::impl()));
            }

            inline std::size_t ID() const override { return base::impl().ID();}

            inline bool enable() override { return base::impl().enable();}
            inline bool disable() override { return base::impl().disable();}

            inline void setUniform(const std::string & name, bool value) override { return base::impl().setUniform(name, value);}

            inline void setUniform(const std::string & name, int value) override { return base::impl().setUniform(name, value);}
            inline void setUniform(const std::string & name, float value) override { return base::impl().setUniform(name, value);}

            inline void setUniform(const std::string & name, const glm::vec2 & value) override { return base::impl().setUniform(name, value);}
            inline void setUniform(const std::string & name, const glm::vec3 & value) override { return base::impl().setUniform(name, value);}
            inline void setUniform(const std::string & name, const glm::vec4 & value) override { return base::impl().setUniform(name, value);}

            inline void setUniform(const std::string & name, const glm::mat2 & value) override { return base::impl().setUniform(name, value);}
            inline void setUniform(const std::string & name, const glm::mat3 & value) override { return base::impl().setUniform(name, value);}
            inline void setUniform(const std::string & name, const glm::mat4 & value) override { return base::impl().setUniform(name, value);}
            inline void setUniform(const std::string & name, const std::vector<glm::mat4> & values) override { return base::impl().setUniform(name, values);}

            inline void setUniform(const std::string & name, unsigned int unit, const ITexture & value) override { return base::impl().setUniform(name, unit, value);}
            inline void setUniform(const std::string & name, unsigned int unit, const std::vector<ITexture*> & values) override { return base::impl().setUniform(name, unit, values);}
    };

    template<class ShaderImplType>
    ShaderModel(ShaderImplType && ) -> ShaderModel<ShaderImplType>;

    class Shader final : public type::Implementation<IShader, ShaderModel> 
    {                                                                   
        public:
            /* move ctor */
            inline Shader(Shader && other) = default;
            inline Shader & operator=(Shader && other) = default;
        
            /* ctor */
            template<class ShaderImplType, class... Args>
            inline Shader(Args && ... args)
                : Implementation(std::in_place_type<ShaderImplType>, std::forward<Args>(args)...)
            {}

            template<class ShaderImplType, class... Args>
            inline Shader(std::in_place_type_t<ShaderImplType>, Args && ... args)
                : Implementation(std::in_place_type<ShaderImplType>, std::forward<Args>(args)...)
            {}
    };

    template <typename H>
    inline H AbslHashValue(H h, const Shader & shader) {
        return H::combine(std::move(h), shader->ID());
    }

    inline bool operator==(const Shader & lhs, const Shader & rhs){
        return lhs->ID() == rhs->ID();
    }
}