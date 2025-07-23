
#pragma once

#include <string>
#include <utility>

#include "type/type.hpp"

#include "render/texture.hpp"

namespace astre::render
{
    // Represents information about attachment to a Frame Buffer Object (FBO)
    struct FBOAttachment
    {
        // Attachment type
        enum class Type
        {
            Texture,
            RenderBuffer
        };

        // Attachment point
        enum class Point
        {
            Color,
            Depth,
            Stencil
        };

        Type type;
        Point point;
        TextureFormat format;
    };

    /**
     * @brief Interface definition for a Frame Buffer Object (FBO)
    */
    class IFrameBufferObject : public type::InterfaceBase
    {
        public:
        //
        virtual ~IFrameBufferObject() = default;
        
        /**
         * @brief Get the ID of the frame buffer object.
         * 
         * @return The ID of the frame buffer object.
         */
        virtual std::size_t ID() const = 0;

        /**
         * @brief Check if the frame buffer object is valid.
         * 
         * @return True if the frame buffer object is valid, false otherwise.
         */
        virtual bool good() const = 0;

        /**
         * @brief Enable the frame buffer object for rendering.
         * 
         * @return True if the frame buffer object was successfully enabled, false otherwise.
         */
        virtual bool enable() const = 0;

        /**
         * @brief Disable the frame buffer object.
         * 
         */
        virtual void disable() const = 0;

        /**
         * @brief Get the textures attached to the frame buffer object.
         * 
         * @return A vector of texture IDs.
         */
        virtual const std::vector<std::size_t> & getTextures() const = 0;
        
        /**
         * @brief Get the resolution of the frame buffer object.
         * 
         * @return The resolution of the frame buffer object.
         */
        virtual std::pair<unsigned int, unsigned int> getResolution() const = 0;
    };

    template<class FrameBufferObjectImplType>
    class FrameBufferObjectModel final : public type::ModelBase<IFrameBufferObject, FrameBufferObjectImplType>
    {
        public:
            using base = type::ModelBase<IFrameBufferObject, FrameBufferObjectImplType>;

            template<class... Args>                                                                             
            inline FrameBufferObjectModel(Args && ... args)     
                : base(std::forward<Args>(args)...)
            {} 

            explicit inline FrameBufferObjectModel(FrameBufferObjectImplType && obj) 
                : base(std::move(obj))
            {}

            inline std::size_t ID() const override { return base::impl().ID();}
            inline bool good() const override { return base::impl().good();}
            inline bool enable() const override { return base::impl().enable();}
            inline void disable() const override { return base::impl().disable();}
            inline const std::vector<std::size_t> & getTextures() const override { return base::impl().getTextures();}
            inline std::pair<unsigned int, unsigned int> getResolution() const override { return base::impl().getResolution();}
    
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

    template<class FrameBufferObjectImplType>
    FrameBufferObjectModel(FrameBufferObjectImplType && ) -> FrameBufferObjectModel<FrameBufferObjectImplType>;

    class FrameBufferObject final : public type::Implementation<IFrameBufferObject, FrameBufferObjectModel> 
    {                                                                   
        public:
            /* move ctor */
            inline FrameBufferObject(FrameBufferObject && other) = default;
            inline FrameBufferObject & operator=(FrameBufferObject && other) = default;
        
            /* ctor */
            template<class FrameBufferObjectImplType, class... Args>
            inline FrameBufferObject(Args && ... args)
                : Implementation(std::in_place_type<FrameBufferObjectImplType>, std::forward<Args>(args)...)
            {}

            /* ctor */
            template<class FrameBufferObjectImplType, class... Args>
            inline FrameBufferObject(std::in_place_type_t<FrameBufferObjectImplType>, Args && ... args)
                : Implementation(std::in_place_type<FrameBufferObjectImplType>, std::forward<Args>(args)...)
            {}

    };

    /**
     * @brief Compute the absolute hash value of a FrameBufferObject.
     * 
     * @param h The hash object.
     * @param vb The FrameBufferObject to be hashed.
     * @return The absolute hash value of the FrameBufferObject.
     */
    template <typename H>
    constexpr inline H AbslHashValue(H h, const FrameBufferObject & vb) {
        return H::combine(std::move(h), vb->ID());
    }

    /**
     * @brief Check if two FrameBufferObjects are equal.
     * 
     * @param lhs The first FrameBufferObject.
     * @param rhs The second FrameBufferObject.
     * @return True if the IDs of the two FrameBufferObjects are equal, false otherwise.
     */
    constexpr inline bool operator==(const FrameBufferObject & lhs, const FrameBufferObject & rhs){
        return lhs->ID() == rhs->ID();
    }
}