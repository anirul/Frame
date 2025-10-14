#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "frame/json/proto.h"
#include "frame/texture_interface.h"

namespace frame::vulkan::json
{

std::unique_ptr<frame::TextureInterface> ParseTexture(
    const frame::proto::Texture& proto_texture, glm::uvec2 size);

} // namespace frame::vulkan::json
