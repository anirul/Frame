#pragma once

#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::json
{

proto::Material SerializeMaterial(
    const MaterialInterface& material_interface,
    const LevelInterface& level_interface);

} // End namespace frame::json.
