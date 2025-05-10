#pragma once

#include "frame/level_interface.h"
#include "frame/json/proto.h"

namespace frame::json
{

/**
 * @brief Serialize a texture to a proto.
 * @param texture_interface: Texture to be serialized.
 * @return Proto of the texture.
 */
proto::Texture SerializeTexture(TextureInterface& texture_interface);

} // End namespace frame::json.
