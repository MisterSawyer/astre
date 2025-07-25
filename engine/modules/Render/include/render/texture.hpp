
#pragma once

#include <string>
#include <utility>

#include "type/type.hpp"

namespace astre::render
{
    /**
     * @brief Texture Format
     * 
     */
    enum class TextureFormat
    {
        RGB_8,
        RGB_16,
        RGB_32,

        RGB_16F,
        RGB_32F,

        RGBA_8,
        RGBA_16,
        RGBA_32,

        RGBA_16F,
        RGBA_32F,

        Depth,
        Depth_32F,
        Stencil
    };
    
    /**
     * @brief Texture Interface
     * 
     */
    class ITexture : public type::InterfaceBase
    {
        public:
        virtual ~ITexture() = default;

        virtual void move(ITexture * dest) = 0;

        /**
         * @brief Get the ID of the texture.
         * 
         * @return The ID of the texture.
         */
        virtual std::size_t ID() const = 0;

        /**
         * @brief Check if the texture is valid.
         * 
         * @return True if the texture is valid, false otherwise.
         */
        virtual bool good() const = 0;

        /**
         * @brief Enable the texture for rendering.
         * 
         * @return True if the texture was successfully enabled, false otherwise.
         */
        virtual bool enable() const = 0;

        /**
         * @brief Disable the texture.
         * 
         */
        virtual void disable() const = 0;
    };

    template<class TextureImplType>
    class TextureModel final : public type::ModelBase<ITexture, TextureImplType>
    {
        public:
            using base = type::ModelBase<ITexture, TextureImplType>;

            template<class... Args>                                                                             
            inline TextureModel(Args && ... args) 
                : base(std::forward<Args>(args)...)
            {} 

            explicit inline TextureModel(TextureImplType && obj) 
                : base(std::move(obj))
            {}

            inline void move(ITexture * dest) override
            {
                ::new(dest) TextureModel(std::move(base::impl()));
            }

            inline std::size_t ID() const override { return base::impl().ID();}
            inline bool good() const override { return base::impl().good();}
            inline bool enable() const override { return base::impl().enable();}
            inline void disable() const override { return base::impl().disable();}
    };

    template<class TextureImplType>
    TextureModel(TextureImplType && ) -> TextureModel<TextureImplType>;

    class Texture : public type::Implementation<ITexture, TextureModel> 
    {                                                                   
        public:
            /* move ctor */
            inline Texture(Texture && other) = default;
            inline Texture & operator=(Texture && other) = default;
        
            /* ctor */
            template<class TextureImplType, class... Args>
            inline Texture(Args && ... args)
                : Implementation(std::in_place_type<TextureImplType>, std::forward<Args>(args)...)
            {}

            /* ctor */
            template<class TextureImplType, class... Args>
            inline Texture(std::in_place_type_t<TextureImplType>, Args && ... args)
                : Implementation(std::in_place_type<TextureImplType>, std::forward<Args>(args)...)
            {}
    };

    /**
     * @brief Compute the absolute hash value of a Texture.
     * 
     * @param h The hash object.
     * @param vb The Texture to be hashed.
     * @return The absolute hash value of the Texture.
     * 
     * @note This function is used to compute the absolute hash value of a Texture.
     * @note The hash value is computed using the absl::Hash API.
     */
    template <typename H>
    inline H AbslHashValue(H h, const Texture & vb) {
        return H::combine(std::move(h), vb->ID());
    }

    /**
     * @brief Check if two Textures are equal.
     * 
     * @param lhs The first Texture.
     * @param rhs The second Texture.
     * @return True if the IDs of the two Textures are equal, false otherwise.
     * 
     * @note This function checks if two Textures are equal by comparing their IDs.
     */
    inline bool operator==(const Texture & lhs, const Texture & rhs){
        return lhs->ID() == rhs->ID();
    }
}