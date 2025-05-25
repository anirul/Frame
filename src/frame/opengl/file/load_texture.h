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

} // namespace frame::opengl::file
