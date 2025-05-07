#pragma once

#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::proto
{

proto::Material SerializeMaterial(
    const MaterialInterface& material_interface,
    const LevelInterface& level_interface);

} // End namespace frame::proto.
