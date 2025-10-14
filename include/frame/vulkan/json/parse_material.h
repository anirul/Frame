#pragma once

#include <memory>

#include "frame/json/proto.h"
#include "frame/level_interface.h"
#include "frame/material_interface.h"

namespace frame::vulkan::json
{

std::unique_ptr<frame::MaterialInterface> ParseMaterial(
    const frame::proto::Material& proto_material,
    frame::LevelInterface& level);

} // namespace frame::vulkan::json
