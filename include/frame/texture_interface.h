#pragma once

#include <any>
#include <cinttypes>
#include <glm/glm.hpp>
#include <utility>
#include "frame/name_interface.h"
#include "frame/proto/proto.h"

namespace frame {

/**
 * @class TextureInterface
 * @brief This class is there to hold a texture (2D or 3D).
 */
struct TextureInterface : public NameInterface {
	//! @brief Virtual destructor.
    virtual ~TextureInterface() = default;
    /**
     * @brief Get the pixel structure (R, RG, RGB, RGBA).
     * @return the pixel structure.
     */
    virtual proto::PixelStructure::Enum GetPixelStructure() const = 0;
    /**
     * @brief Get the pixel element size individual element (BYTE, SHORT, LONG, FLOAT).
     * @return The pixel element size.
     */
    virtual proto::PixelElementSize::Enum GetPixelElementSize() const = 0;
    /**
     * @brief Get the size of the current texture.
     * @return The size of the texture.
     */
    virtual std::pair<std::uint32_t, std::uint32_t> GetSize() const = 0;
    //! @brief Enable mipmap, this allow a recursive level of texture faster for rendering.
    virtual void EnableMipmap() const                                 = 0;
    /**
     * @brief Set the minification filter.
     * @param texture_filter: Usually and by default GL_LINEAR.
     */
    virtual void SetMinFilter(const proto::TextureFilter::Enum texture_filter) = 0;
    /**
     * @brief Get the minification filter.
     * @return The value of the minification filter.
     */
    virtual proto::TextureFilter::Enum GetMinFilter() const = 0;
    /**
     * @brief Set the magnification filter.
     * @param texture_filter: Usually and by default GL_LINEAR.
     */
    virtual void SetMagFilter(const proto::TextureFilter::Enum texture_filter) = 0;
    /**
     * @brief Get the magnification filter.
     * @return The value of the magnification filter.
     */
    virtual proto::TextureFilter::Enum GetMagFilter() const                    = 0;
    /**
     * @brief Set the wrapping on the s size of the texture (horizontal) this will decide how the
     * texture is treated in case you overflow in this direction.
     * @param texture_filter: Could be any of (REPEAT, CLAMP_TO_EDGE, MIRRORED_REPEAT).
     */
    virtual void SetWrapS(const proto::TextureFilter::Enum texture_filter)     = 0;
    /**
     * @brief Get the wrapping on the s size of the texture (horizontal).
     * @return The way the texture is wrap could be any of (REPEAT, CLAMP_TO_EDGE, MIRRORED_REPEAT).
     */
    virtual proto::TextureFilter::Enum GetWrapS() const                        = 0;
    /**
     * @brief Set the wrapping on the t size of the texture (vertical) this will decide how the
     * texture is treated in case you overflow in this direction.
     * @param texture_filter: Could be any of (REPEAT, CLAMP_TO_EDGE, MIRRORED_REPEAT).
     */
    virtual void SetWrapT(const proto::TextureFilter::Enum texture_filter) = 0;
    /**
     * @brief Get the wrapping on the t size of the texture (vertical).
     * @return The way the texture is wrap could be any of (REPEAT, CLAMP_TO_EDGE, MIRRORED_REPEAT).
     */
    virtual proto::TextureFilter::Enum GetWrapT() const = 0;
    /**
     * @brief Clear the texture (this is highly inefficient).
     * @param color: Color to paint the texture to.
     */
    virtual void Clear(const glm::vec4 color) = 0;
    /**
     * @brief This return if the texture is a cube map or not.
     * @return In this case this is false.
     */
    virtual bool IsCubeMap() const = 0;
    /**
     * @brief Get a copy of the texture output (8 bit format).
     * @return A vector containing the pixel of the image in 8 bit format.
     */
    virtual std::vector<std::uint8_t> GetTextureByte() const = 0;
    /**
     * @brief Get a copy of the texture output (16 bit format).
     * @return A vector containing the pixel of the image in 16 bit format.
     */
    virtual std::vector<std::uint16_t> GetTextureWord() const = 0;
    /**
     * @brief Get a copy of the texture output (32 bit format).
     * @return A vector containing the pixel of the image in 32 bit format.
     */
    virtual std::vector<std::uint32_t> GetTextureDWord() const = 0;
    /**
     * @brief Get a copy of the texture output (32 bit float format).
     * @return A vector containing the pixel of the image in 32 bit float format.
     */
    virtual std::vector<float> GetTextureFloat() const = 0;
};

}  // End namespace frame.
