#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "frame/image_interface.h"
#include "frame/json/parse_pixel.h"

namespace frame::file
{

/**
 * @class Image
 * @brief This class is the representation of an image that come (or go) to
 *        a file on the HDD.
 *
 * The idea is to load an image and save it on the GPU using the texture
 * class.
 */
class Image : public ImageInterface
{
  public:
    /**
     * @brief Constructor this will build a image file from a size and image
     * specs, this will be use to save image to the HDD.
     * @param size: Size of the image.
     * @param pixel_element_size: Size of one of the element in a pixel
     *        (BYTE, SHORT, HALF, FLOAT).
     * @param pixel_element_structure: Structure of a pixel (R, RG, RGB,
     *        RGBA).
     */
    Image(
        glm::uvec2 size,
        proto::PixelElementSize pixel_element_size =
            json::PixelElementSize_BYTE(),
        proto::PixelStructure pixel_structure = json::PixelStructure_RGB());
    /**
     * @brief Constructor this will build a image from a file and image
     *        specs, this will be use to load an image from the HDD.
     * @param size: Size of the image.
     * @param pixel_element_size: Size of one of the element in a pixel
     *        (BYTE, SHORT, HALF, FLOAT).
     * @param pixel_element_structure: Structure of a pixel (R, RG, RGB,
     *        RGBA).
     */
    Image(
        const std::filesystem::path& file,
        proto::PixelElementSize pixel_element_size =
            json::PixelElementSize_BYTE(),
        proto::PixelStructure pixel_structure = json::PixelStructure_RGB());
    //! Virtual destructor.
    ~Image();

  public:
    /**
     * @brief Get size of the image.
     * @return The size of the image.
     */
    glm::uvec2 GetSize() const override
    {
        return glm::uvec2(size_);
    }
    /**
     * @brief Get the length of the image (should be size.x * size.y).
     * @return The size of the image in linear.
     */
    int GetLength() const override
    {
        return size_.x * size_.y;
    }
    /**
     * @brief Get a pointer to the underlying structure.
     * @return A void pointer to the data structure.
     */
    const void* Data() const override
    {
        return image_;
    }
    /**
     * @brief Get a pointer to the underlying structure.
     * @return A void pointer to the data structure.
     */
    void* Data() override
    {
        return image_;
    }
    /**
     * @brief Get pixel element size (BYTE/SHOT/HALF/FLOAT).
     * @return The proto pixel element size.
     */
    proto::PixelElementSize GetPixelElementSize() const override
    {
        return pixel_element_size_;
    }
    /**
     * @brief Get pixel structure (R/RG/RGB/RGBA).
     * @return The proto pixel structure.
     */
    proto::PixelStructure GetPixelStructure() const override
    {
        return pixel_structure_;
    }

  public:
    /**
     * @brief Save an image to a file.
     * @param file: The file to save the image to.
     */
    void SaveImageToFile(const std::string& file) const override;
    /**
     * @brief Set the data pointer to the class, used to be able to save to
     *        HDD.
     * @param data: Pointer to the memory containing the RGB data of the
     *        image.
     * @warning Note that the class won't be the owner of the pointer so it
     *          won't free it when destroyed!
     */
    void SetData(void* data) override;

  private:
    glm::ivec2 size_ = glm::ivec2(0, 0);
    bool free_ = false;
    void* image_ = nullptr;
    const proto::PixelElementSize pixel_element_size_;
    const proto::PixelStructure pixel_structure_;
};

} // End of namespace frame::file.
