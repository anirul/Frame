#pragma once

#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "frame/json/parse_pixel.h"
#include "frame/json/proto.h"
#include "frame/opengl/frame_buffer.h"
#include "frame/opengl/pixel.h"
#include "frame/opengl/program.h"
#include "frame/opengl/render_buffer.h"
#include "frame/opengl/scoped_bind.h"
#include "frame/texture_interface.h"

namespace frame::opengl
{

/**
 * @class Cubemap
 * @brief The same as texture but in a cube map perfect for environment
 * maps.
 */
class Cubemap : public TextureInterface, public BindInterface
{
  public:
    /**
     * @brief Create a texture from a proto.
     * @param proto_parameter: The proto to guide in the creation of the
     * texture.
     */
    Cubemap(const proto::Texture& proto_texture);
    /**
     * @brief Create a texture from a file.
     * @param file_name: A file to be loaded into the texture.
     * @param pixel_element_size: The element size of the texture.
     * @param pixel_structure: The pixel structure of the texture.
     */
    Cubemap(
        std::filesystem::path file_name,
        proto::PixelElementSize pixel_element_size,
        proto::PixelStructure pixel_strucutre);
    /**
     * @brief Create from 6 textures (cubemap).
     * @param file_names: 6 files to describe this new cubemap.
     * @param pixel_element_size: The element size of the texture.
     * @param pixel_structure: The pixel structure of the texture.
     */
    Cubemap(
        std::array<std::filesystem::path, 6> file_names,
        proto::PixelElementSize pixel_element_size,
        proto::PixelStructure pixel_strucutre);
    /**
     * @brief Create a texture from a pointer and a brief description of the
     * underlying structure.
     * @param ptr: The pointer to the underlying structure.
     * @param size: The size of the texture.
     * @param pixel_element_size: The element size of the texture.
     * @param pixel_structure: The pixel structure of the texture.
     */
    Cubemap(
        const void* ptr,
        glm::uvec2 size,
        proto::PixelElementSize pixel_element_size,
        proto::PixelStructure pixel_structure);
    /**
     * @brief Create a texture from a pointer and a brief description of the
     * underlying structure.
     * @param ptrs: The pointers to the underlying structures.
     * @param size: The size of the texture.
     * @param pixel_element_size: The element size of the texture.
     * @param pixel_structure: The pixel structure of the texture.
     */
    Cubemap(
        std::array<const void*, 6> ptrs,
        glm::uvec2 size,
        proto::PixelElementSize pixel_element_size,
        proto::PixelStructure pixel_structure);
    //! @brief Destroy texture also on GPU side.
    ~Cubemap();

  public:
    /**
     * @brief Convert texture filter enum to GL type.
     * @param texture_filter: Proto filter to be converted to OpenGL.
     * @return OpenGL value.
     */
    int ConvertToGLType(proto::TextureFilter::Enum texture_filter) const;
    /**
     * @brief Convert texture OpenGL to filter enum.
     * @param gl_filter: OpenGL value to be converted.
     * @return Proto texture filter.
     */
    proto::TextureFilter::Enum ConvertFromGLType(int gl_filter) const;
    /**
     * @brief Get a copy of the texture output (8 bit format).
     * @return A vector containing the pixel of the image in 8 bit format.
     */
    std::vector<std::uint8_t> GetTextureByte() const override;
    /**
     * @brief Get a copy of the texture output (16 bit format).
     * @return A vector containing the pixel of the image in 16 bit format.
     */
    std::vector<std::uint16_t> GetTextureWord() const override;
    /**
     * @brief Get a copy of the texture output (32 bit format).
     * @return A vector containing the pixel of the image in 32 bit format.
     */
    std::vector<std::uint32_t> GetTextureDWord() const override;
    /**
     * @brief Get a copy of the texture output (32 bit float format).
     * @return A vector containing the pixel of the image in 32 bit float
     * format.
     */
    std::vector<float> GetTextureFloat() const override;
    /**
     * @brief Clear the texture (this is highly inefficient).
     * @param color: Color to paint the texture to.
     */
    void Clear(glm::vec4 color) override;
    /**
     * @brief From the bind interface this will bind the texture at slot to
     * the current context.
     * @param slot: Slot to be binded.
     */
    void Bind(unsigned int slot = 0) const override;
    /**
     * @brief From the bind interface this will unbind the current texture
     * from the context.
     */
    void UnBind() const override;
    /**
     * @brief Copy the texture input to the texture.
     * @param vector: Vector of uint32_t containing the RGBA values of the
     * texture.
     * @param size: Size of the image.
     */
    void Update(
        std::vector<std::uint8_t>&& vector,
        glm::uvec2 size,
        std::uint8_t bytes_per_pixel) override;
    //! @brief Enable mipmap generation.
    void EnableMipmap() override;
    /**
     * @brief Get the computed size (can be different from the stored one).
     * @return The computed size.
     */
    glm::uvec2 GetSize() override;
    /**
     * @brief Set the window size (in case the
     * texture size is relative).
     * @param size: The window size.
     */
    void SetWindowSize(glm::uvec2 size) override;

  public:
    /**
     * @brief Get texture frame from position.
     * @param i: Position (+X, -X, +Y, -Y, +Z, -Z) [0, 5].
     * @return The texture frame.
     */
    static proto::TextureFrame GetTextureFrameFromPosition(int i);

  public:
    /**
     * @brief Return the texture cube map OpenGL id, from the bind
     * interface.
     * @return Get the OpenGL id of the texture.
     */
    unsigned int GetId() const override
    {
        return texture_id_;
    }

  protected:
    /**
     * @brief Protected constructor this is a way to create a simple
     * structure from inside the class.
     * @param pixel_element_size: Size of one of the element in a pixel
     * (BYTE, SHORT, HALF, FLOAT).
     * @param pixel_element_structure: Structure of a pixel (R, RG, RGB,
     * RGBA).
     */
    Cubemap(
        const proto::PixelElementSize pixel_element_size =
            json::PixelElementSize_BYTE(),
        const proto::PixelStructure pixel_structure =
            json::PixelStructure_RGB())
    {
        data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
        data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    }
    //! @brief Lock the bind for RAII interface to the bind interface.
    void LockedBind() const override
    {
        locked_bind_ = true;
    }
    //! @brief Unlock the bind for RAII interface to the bind interface.
    void UnlockedBind() const override
    {
        locked_bind_ = false;
    }

  protected:
    //! Create a render and a frame buffer for internal rendering (used in
    //! Clear).
    void CreateFrameAndRenderBuffer();
    /**
     * @brief Create a cubemap from a single file.
     * @param file_name: File that will create the texture.
     * @param pixel_element_size: The element size of the texture.
     * @param pixel_structure: The pixel structure of the texture.
     */
    void CreateCubemapFromFile(
        std::filesystem::path file_name,
        proto::PixelElementSize pixel_element_size,
        proto::PixelStructure pixel_strucutre);
    /**
     * @brief Create a cubemap from 6 files.
     * @param file_names: Files that will create the texture.
     * @param pixel_element_size: The element size of the texture.
     * @param pixel_structure: The pixel structure of the texture.
     */
    void CreateCubemapFromFiles(
        std::array<std::filesystem::path, 6> file_names,
        proto::PixelElementSize pixel_element_size,
        proto::PixelStructure pixel_structure);
    /**
     * @brief Create a texture from a pointer and a brief description of the
     * underlying structure.
     * @param ptr: The pointer to the underlying structure.
     * @param size: The size of the texture.
     * @param pixel_element_size: The element size of the texture.
     * @param pixel_structure: The pixel structure of the texture.
     */
    void CreateCubemapFromPointer(
        const void* ptr,
        glm::uvec2 size,
        proto::PixelElementSize pixel_element_size,
        proto::PixelStructure pixel_structure);
    /**
     * @brief Create a texture from a pointer and a brief description of the
     * underlying structure.
     * @param ptrs: The pointers to the underlying structures.
     * @param size: The size of the texture.
     * @param pixel_element_size: The element size of the texture.
     * @param pixel_structure: The pixel structure of the texture.
     */
    void CreateCubemapFromPointers(
        std::array<const void*, 6> ptrs,
        glm::uvec2 size,
        proto::PixelElementSize pixel_element_size,
        proto::PixelStructure pixel_structure);

  protected:
    friend class ScopedBind;

  private:
    unsigned int texture_id_ = 0;
    mutable bool locked_bind_ = false;
    std::unique_ptr<RenderBuffer> render_ = nullptr;
    std::unique_ptr<FrameBuffer> frame_ = nullptr;
    glm::uvec2 inner_size_;
};

} // End namespace frame::opengl.
