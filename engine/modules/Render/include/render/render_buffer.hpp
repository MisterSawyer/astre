
#pragma once

#include <utility>
#include <string>

#include "type/type.hpp"

namespace astre::render
{
    class IRenderBuffer : public type::InterfaceBase
    {
        public:
        virtual ~IRenderBuffer() = default;

        /**
         *  @brief Get the ID of the render buffer.
         *  
         *  @return The ID of the render buffer.
         */
        virtual std::size_t ID() const = 0;

        /**
         *  @brief Check if the render buffer is valid.
         *  
         *  @return True if the render buffer is valid, false otherwise.
         */
        virtual bool good() const = 0;

        /**
         *  @brief Enable the render buffer for rendering.
         *  
         *  @return True if the render buffer was successfully enabled, false otherwise.
         */
        virtual bool enable() const = 0;

        /**
         *  @brief Disable the render buffer.
         *  
         */
        virtual void disable() const = 0;
    };

    template<class RenderBufferImplType>
    class RenderBufferModel final : public type::ModelBase<IRenderBuffer, RenderBufferImplType>
    {
        public:
            using base = type::ModelBase<IRenderBuffer, RenderBufferImplType>;

            template<class... Args>                                                                             
            inline RenderBufferModel(Args && ... args)  
                : base(std::forward<Args>(args)...)
            {} 

            explicit inline RenderBufferModel(RenderBufferImplType && obj)  
                : base(std::move(obj))
            {}

            inline std::size_t ID() const override { return base::impl().ID();}
            inline bool good() const override { return base::impl().good();}
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

    template<class RenderBufferImplType>
    RenderBufferModel(RenderBufferImplType && ) -> RenderBufferModel<RenderBufferImplType>;

    class RenderBuffer : public type::Implementation<IRenderBuffer, RenderBufferModel> 
    {                                                                   
        public:
            /* move ctor */
            inline RenderBuffer(RenderBuffer && other) = default;
            inline RenderBuffer & operator=(RenderBuffer && other) = default;
        
            /* ctor */
            template<class RenderBufferImplType, class... Args>
            inline RenderBuffer(Args && ... args)
                : Implementation(std::in_place_type<RenderBufferImplType>, std::forward<Args>(args)...)
            {}

            /* ctor */
            template<class RenderBufferImplType, class... Args>
            inline RenderBuffer(std::in_place_type_t<RenderBufferImplType>, Args && ... args)
                : Implementation(std::in_place_type<RenderBufferImplType>, std::forward<Args>(args)...)
            {}
    };

    /**
     *  @brief Compute the absolute hash value of a RenderBuffer.
     *  
     *  @param h The hash object.
     *  @param vb The RenderBuffer to be hashed.
     *  @return The absolute hash value of the RenderBuffer.
     *  
     *  @note This function is used to compute the absolute hash value of a RenderBuffer.
     *  @note The hash value is computed using the absl::Hash API.
     */
    template <typename H>
    constexpr inline H AbslHashValue(H h, const RenderBuffer & vb) {
        return H::combine(std::move(h), vb->ID());
    }

    /**
     * @brief Check if two RenderBuffers are equal.
     * 
     * @param lhs The first RenderBuffer.
     * @param rhs The second RenderBuffer.
     * @return True if the IDs of the two RenderBuffers are equal, false otherwise.
     */
    constexpr inline bool operator==(const RenderBuffer & lhs, const RenderBuffer & rhs){
        return lhs->ID() == rhs->ID();
    }
}