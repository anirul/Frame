#pragma once

#include <any>
#include <array>
#include <cinttypes>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <utility>

#include "frame/json/parse_pixel.h"
#include "frame/json/proto.h"
#include "frame/name_interface.h"

namespace frame
{

/**
 * @class TextureTypeEnum
 * @brief The type of texture you want to create.
 */
enum class TextureTypeEnum
{
    TEXTURE_2D,
    TEXTURE_3D,
    CUBMAP,
};

/**
 * @class TextureParameter
 * @brief This is the parameters needed to create a new texture.
 * @note TODO(anirul): Change cubemap to a enum and add a parameter for
 *       mipmap.
 */
struct TextureParameter
{
    //! @brief Pixel element size, this should be the same as sizeof(T).
    proto::PixelElementSize pixel_element_size = proto::PixelElementSize_BYTE();
    /**
     * @brief Pixel structure, this is the number of color you have in the
     *        texture 1 to 4.
     */
    proto::PixelStructure pixel_structure = proto::PixelStructure_RGB();
    /**
     * @brief Texture size, in case of cubemap this is the value of a single
     *        plane.
     */
    glm::uvec2 size = glm::uvec2(1, 1);
    /**
     * @brief Texture data, in case you don't want to provide it just pass
     *        an empty vector. You have to multiply the size by the pixel
     *        element size to get the size of the vector.
     */
    void* data_ptr = nullptr;
    //! @brief In case of cube map you need 6 planes of array pointers.
    std::array<void*, 6> array_data_ptr = {
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    //! @brief Is it a cube map or a normal 2d texture.
    TextureTypeEnum map_type = TextureTypeEnum::TEXTURE_2D;
};

/**
 * @class TextureInterface
 * @brief This class is there to hold a texture (2D or 3D).
 * In case you want to create a texture, you should use the device.
 */
struct TextureInterface : public NameInterface
{
    //! @brief Virtual destructor.
    virtual ~TextureInterface() = default;
    /**
	 * @brief Get the texture parameters used at creation, usefull for
	 * serialization.
	 * @return Texture parameters used at creation.
	 */
    virtual const TextureParameter& GetTextureParameter() const = 0;
    /**
     * @brief Get the pixel structure (R, RG, RGB, RGBA).
     * @return the pixel structure.
     */
    virtual proto::PixelStructure::Enum GetPixelStructure() const = 0;
    /**
     * @brief Get the pixel element size individual element (BYTE, SHORT,
     *        LONG, FLOAT).
     * @return The pixel element size.
     */
    virtual proto::PixelElementSize::Enum GetPixelElementSize() const = 0;
    /**
     * @brief Get the size of the current texture.
     * @return The size of the texture.
     */
    virtual glm::uvec2 GetSize() const = 0;
    /**
     * @brief Enable mipmap, this allow a recursive level of texture faster
     *        for rendering.
     */
    virtual void EnableMipmap() const = 0;
    /**
     * @brief Set the minification filter.
     * @param texture_filter: Usually and by default GL_LINEAR.
     */
    virtual void SetMinFilter(
        const proto::TextureFilter::Enum texture_filter) = 0;
    /**
     * @brief Get the minification filter.
     * @return The value of the minification filter.
     */
    virtual proto::TextureFilter::Enum GetMinFilter() const = 0;
    /**
     * @brief Set the magnification filter.
     * @param texture_filter: Usually and by default GL_LINEAR.
     */
    virtual void SetMagFilter(
        const proto::TextureFilter::Enum texture_filter) = 0;
    /**
     * @brief Get the magnification filter.
     * @return The value of the magnification filter.
     */
    virtual proto::TextureFilter::Enum GetMagFilter() const = 0;
    /**
     * @brief Set the wrapping on the s size of the texture (horizontal)
     *        this will decide how the texture is treated in case you
     *        overflow in this direction.
     * @param texture_filter: Could be any of (REPEAT, CLAMP_TO_EDGE,
     *        MIRRORED_REPEAT).
     */
    virtual void SetWrapS(const proto::TextureFilter::Enum texture_filter) = 0;
    /**
     * @brief Get the wrapping on the s size of the texture (horizontal).
     * @return The way the texture is wrap could be any of (REPEAT,
     *         CLAMP_TO_EDGE, MIRRORED_REPEAT).
     */
    virtual proto::TextureFilter::Enum GetWrapS() const = 0;
    /**
     * @brief Set the wrapping on the t size of the texture (vertical) this
     *        will decide how the texture is treated in case you overflow in
     *        this direction.
     * @param texture_filter: Could be any of (REPEAT, CLAMP_TO_EDGE,
     *        MIRRORED_REPEAT).
     */
    virtual void SetWrapT(const proto::TextureFilter::Enum texture_filter) = 0;
    /**
     * @brief Get the wrapping on the t size of the texture (vertical).
     * @return The way the texture is wrap could be any of (REPEAT,
     *         CLAMP_TO_EDGE, MIRRORED_REPEAT).
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
     * @return A vector containing the pixel of the image in 32 bit float
     *         format.
     */
    virtual std::vector<float> GetTextureFloat() const = 0;
    /**
     * @brief Copy the texture input to the texture.
     * @param vector: Vector of uint32_t containing the RGBA values of the
     *        texture.
     * @param size: Size of the image.
     */
    virtual void Update(
        std::vector<std::uint8_t>&& vector,
        glm::uvec2 size,
        std::uint8_t bytes_per_pixel) = 0;
};

} // End namespace frame.
