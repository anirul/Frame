#pragma once

#include <memory>
#include <optional>

#include "frame/json/proto.h"
#include "frame/texture_interface.h"

namespace frame::json
{

/**
 * @brief Parse a proto to a texture object.
 * @param proto_texture: Proto file containing the texture description
 * @param size: Size of the texture.
 * @return A unique pointer to an Texture interface.
 */
std::unique_ptr<TextureInterface> ParseTexture(
    const proto::Texture& proto_texture, glm::uvec2 size);
/**
 * @brief Parse a proto to a texture cubemap object.
 * @param proto_texture: Proto file containing the texture description
 * @param size: Size of the texture.
 * @return A unique pointer to an Texture interface.
 */
std::unique_ptr<TextureInterface> ParseCubemap(
    const proto::Texture& proto_texture, glm::uvec2 size);

} // End namespace frame::json.
