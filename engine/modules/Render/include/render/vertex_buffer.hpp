
#pragma once

#include <utility>
#include <string>

#include "type/type.hpp"

namespace astre::render
{
    /**
     * @brief Vertex Buffer Interface
     * 
     */
    class IVertexBuffer : public type::InterfaceBase
    {
        public:
        virtual ~IVertexBuffer() = default;

        /**
         * @brief Get the ID of the vertex buffer.
         * 
         * @return The ID of the vertex buffer.
         */
        virtual std::size_t ID() const = 0;

        /**
         * @brief Check if the vertex buffer is valid.
         * 
         * @return True if the vertex buffer is valid, false otherwise.
         */
        virtual bool good() const = 0;

        /**
         * @brief Get the number of elements in the vertex buffer.
         * 
         * @return The number of elements in the vertex buffer.
         */
        virtual std::size_t numberOfElements() const = 0;

        /**
         * @brief Enable the vertex buffer for rendering.
         * 
         * @return True if the vertex buffer was successfully enabled, false otherwise.
         */
        virtual bool enable() const = 0;

        /**
         * @brief Disable the vertex buffer.
         * 
         */
        virtual void disable() const = 0;
    };

    template<class VertexBufferImplType>
    class VertexBufferModel final : public type::ModelBase<IVertexBuffer, VertexBufferImplType>
    {
        public:
            using base = type::ModelBase<IVertexBuffer, VertexBufferImplType>;

            template<class... Args>                                                                             
            inline VertexBufferModel(Args && ... args) 
                : base(std::forward<Args>(args)...)
            {} 

            explicit inline VertexBufferModel(VertexBufferImplType && obj) 
                : base(std::move(obj))
            {}

            inline std::size_t ID() const override { return base::impl().ID();}
            inline bool good() const override { return base::impl().good();}
            inline std::size_t numberOfElements() const override { return base::impl().numberOfElements();}
            inline bool enable() const override { return base::impl().enable();}
            inline void disable() const override { return base::impl().disable();}
    
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

    template<class VertexBufferImplType>
    VertexBufferModel(VertexBufferImplType && ) -> VertexBufferModel<VertexBufferImplType>;

    class VertexBuffer : public type::Implementation<IVertexBuffer, VertexBufferModel> 
    {                                                                   
        public:
            /* move ctor */
            inline VertexBuffer(VertexBuffer && other) = default;
            inline VertexBuffer & operator=(VertexBuffer && other) = default;
        
            /* ctor */
            template<class VertexBufferImplType, class... Args>
            inline VertexBuffer(Args && ... args)
                : Implementation(std::in_place_type<VertexBufferImplType>, std::forward<Args>(args)...)
            {}

            /* ctor */
            template<class VertexBufferImplType, class... Args>
            inline VertexBuffer(std::in_place_type_t<VertexBufferImplType>, Args && ... args)
                : Implementation(std::in_place_type<VertexBufferImplType>, std::forward<Args>(args)...)
            {}

    };

    /**
     * @brief Compute the absolute hash value of a VertexBuffer.
     * 
     * @param h The hash object.
     * @param vb The VertexBuffer to be hashed.
     * @return The absolute hash value of the VertexBuffer.
     * 
     * @note This function is used to compute the absolute hash value of a VertexBuffer.
     * @note The hash value is computed using the absl::Hash API.
     */
    template <typename H>
    constexpr inline H AbslHashValue(H h, const VertexBuffer & vb) {
        return H::combine(std::move(h), vb->ID());
    }

    /**
     * @brief Check if two VertexBuffers are equal.
     * 
     * @param lhs The first VertexBuffer.
     * @param rhs The second VertexBuffer.
     * @return True if the IDs of the two VertexBuffers are equal, false otherwise.
     */
    constexpr inline bool operator==(const VertexBuffer & lhs, const VertexBuffer & rhs){
        return lhs->ID() == rhs->ID();
    }
}