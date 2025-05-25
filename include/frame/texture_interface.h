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
#include "frame/serialize.h"

namespace frame
{

/**
 * @class TextureInterface
 * @brief This class is there to hold a texture (2D or 3D).
 * In case you want to create a texture, you should use the device.
 */
struct TextureInterface : public Serialize<proto::Texture>
{
    //! @brief Virtual destructor.
    virtual ~TextureInterface() = default;
    /**
     * @brief Clear the texture (this is highly inefficient).
     * @param color: Color to paint the texture to.
     */
    virtual void Clear(const glm::vec4 color) = 0;
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
     * format.
     */
    virtual std::vector<float> GetTextureFloat() const = 0;
    /**
     * @brief Copy the texture input to the texture.
     * @param vector: Vector of uint32_t containing the RGBA values of the
     * texture.
     * @param size: Size of the image.
     */
    virtual void Update(
        std::vector<std::uint8_t>&& vector,
        glm::uvec2 size,
        std::uint8_t bytes_per_pixel) = 0;
    //! @brief Enable mipmap generation.
    virtual void EnableMipmap() = 0;
    /**
	 * @brief Get the computed size (can be different from the stored one).
	 * @return The computed size.
	 */
    virtual glm::uvec2 GetSize() = 0;
    /**
	 * @brief Set the display size (in case the texture size is relative).
	 * @param display_size: The display size.
	 */
    virtual void SetDisplaySize(glm::uvec2 display_size) = 0;
};

} // End namespace frame.
