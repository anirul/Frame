#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "frame/json/parse_pixel.h"
#include "frame/opengl/pixel.h"
#include "frame/texture_interface.h"

namespace frame::opengl::file
{

/**
 * @brief Load texture from vec4 containing a color.
 * @param vec4: A glm vec4 containing a color definition.
 * @return A unique pointer to the texture interface (or null in case of
 *         failure).
 */
std::unique_ptr<TextureInterface> LoadTextureFromVec4(const glm::vec4& vec4);
/**
 * @brief Load texture from a float.
 * @param f: A float.
 * @return A unique pointer to the texture interface (or null in case of
 *         failure).
 */
std::unique_ptr<TextureInterface> LoadTextureFromFloat(float f);
/**
 * @brief Load texture from file (*.png).
 * @param file: An image file (should be accessible from the current
 *        location).
 * @param pixel_element_size: Size of one of the element in a pixel (BYTE,
 *        SHORT, HALF, FLOAT).
 * @param pixel_element_structure: Structure of a pixel (R, RG, RGB, RGBA).
 * @return A unique pointer to the texture interface (or null in case of
 *         failure).
 */
std::unique_ptr<TextureInterface> LoadTextureFromFile(
    const std::filesystem::path& file,
    proto::PixelElementSize pixel_element_size = proto::PixelElementSize_BYTE(),
    proto::PixelStructure pixel_structure = proto::PixelStructure_RGB());
/**
 * @brief Load texture cube map from a file (*.hdr).
 * @param file: An image file (should be accessible from the current
 *        location).
 * @param pixel_element_size: Size of one of the element in a pixel (BYTE,
 *        SHORT, HALF, FLOAT).
 * @param pixel_element_structure: Structure of a pixel (R, RG, RGB, RGBA).
 * @return A unique pointer to the texture interface (or null in case of
 *         failure).
 */
std::unique_ptr<TextureInterface> LoadCubeMapTextureFromFile(
    const std::filesystem::path& file,
    proto::PixelElementSize pixel_element_size = proto::PixelElementSize_BYTE(),
    proto::PixelStructure pixel_structure = proto::PixelStructure_RGB());
/**
 * @brief Load texture from files.
 * @param files: 6 images files (should be accessible from the current
 *        location).
 * @param pixel_element_size: Size of one of the element in a pixel (BYTE,
 *        SHORT, HALF, FLOAT).
 * @param pixel_element_structure: Structure of a pixel (R, RG, RGB, RGBA).
 * @return A unique pointer to the texture interface (or null in case of
 *         failure).
 */
std::unique_ptr<TextureInterface> LoadCubeMapTextureFromFiles(
    const std::array<std::filesystem::path, 6>& files,
    proto::PixelElementSize pixel_element_size = proto::PixelElementSize_BYTE(),
    proto::PixelStructure pixel_structure = proto::PixelStructure_RGB());

} // namespace frame::opengl::file
