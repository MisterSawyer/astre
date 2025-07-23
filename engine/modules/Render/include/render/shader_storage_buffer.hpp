
#pragma once

#include <utility>
#include <string>

#include "type/type.hpp"

namespace astre::render
{
    /**
     * @brief Interface for Shader Storage Buffer Objects (SSBO)
     * 
     * @note This interface is used to create and manage Shader Storage Buffer Objects (SSBO).
     * 
     */
    class IShaderStorageBuffer : public type::InterfaceBase
    {
        public:
        virtual ~IShaderStorageBuffer() = default;

        /**
         * @brief Get the ID of the shader storage buffer.
         * 
         * @return The ID of the shader storage buffer.
         * 
         * @note This function must be called on the render thread strand.
         */
        virtual std::size_t ID() const = 0;
        
        /**
         * @brief Check if the shader storage buffer is valid.
         * 
         * @return True if the shader storage buffer is valid, false otherwise.
         * 
         * @note This function must be called on the render thread strand.
         */
        virtual bool good() const = 0;

        /**
         * @brief Enable the shader storage buffer.
         * 
         * @return True if the shader storage buffer was successfully enabled, false otherwise.
         * 
         * @note This function must be called on the render thread strand.
         */
        virtual bool enable() const = 0;

        /**
         * @brief Disable the shader storage buffer.
         * 
         * @note This function must be called on the render thread strand.
         */
        virtual void disable() const = 0;

        /**
         * @brief Update the data of the shader storage buffer.
         * 
         * @param size The size of the data to update.
         * @param data The data to update.
         * 
         * @note This function must be called on the render thread strand.
         */
        virtual void update(std::size_t size, const void * data) = 0;
    };

    template<class ShaderStorageBufferImplType>
    class ShaderStorageBufferModel final : public type::ModelBase<IShaderStorageBuffer, ShaderStorageBufferImplType>
    {
        public:
            using base = type::ModelBase<IShaderStorageBuffer, ShaderStorageBufferImplType>;

            template<class... Args>                                                                             
            inline ShaderStorageBufferModel(Args && ... args)   
                : base(std::forward<Args>(args)...)
            {} 

            explicit inline ShaderStorageBufferModel(ShaderStorageBufferImplType && obj) 
                : base(std::move(obj))
            {}

            inline std::size_t ID() const override { return base::impl().ID();}
            inline bool good() const override { return base::impl().good();}
            inline bool enable() const override { return base::impl().enable();}
            inline void disable() const override { return base::impl().disable();}
            inline void update(std::size_t size, const void * data) override { return base::impl().update(std::move(size), std::move(data));}
    
            inline void move(type::InterfaceBase * dest) override
            {
                throw std::runtime_error("Not moveable");
            }

            inline void copy([[maybe_unused]] type::InterfaceBase * dest) const override
            {
                throw std::runtime_error("Not copyable");
            }

            inline std::unique_ptr<type::InterfaceBase> clone() const override
            {
                throw std::runtime_error("Not copyable");
            }
    };

    template<class ShaderStorageBufferImplType>
    ShaderStorageBufferModel(ShaderStorageBufferImplType && ) -> ShaderStorageBufferModel<ShaderStorageBufferImplType>;

    class ShaderStorageBuffer final : public type::Implementation<IShaderStorageBuffer, ShaderStorageBufferModel> 
    {                                                                   
        public:
            /* move ctor */
            inline ShaderStorageBuffer(ShaderStorageBuffer && other) = default;
            inline ShaderStorageBuffer & operator=(ShaderStorageBuffer && other) = default;
        
            /* ctor */
            template<class ShaderStorageBufferImplType, class... Args>
            inline ShaderStorageBuffer(Args && ... args)
                : Implementation(std::in_place_type<ShaderStorageBufferImplType>, std::forward<Args>(args)...)
            {}

            /* ctor */
            template<class ShaderStorageBufferImplType, class... Args>
            inline ShaderStorageBuffer(std::in_place_type_t<ShaderStorageBufferImplType>, Args && ... args)
                : Implementation(std::in_place_type<ShaderStorageBufferImplType>, std::forward<Args>(args)...)
            {}  

    };

    template <typename H>
    constexpr inline H AbslHashValue(H h, const ShaderStorageBuffer & vb) {
        return H::combine(std::move(h), vb->ID());
    }

    constexpr inline bool operator==(const ShaderStorageBuffer & lhs, const ShaderStorageBuffer & rhs){
        return lhs->ID() == rhs->ID();
    }
}