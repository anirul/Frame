#pragma once

#include "frame/level_interface.h"
#include "frame/json/proto.h"

namespace frame::proto
{

/**
 * @brief Serialize a level into a proto.
 * @param level: Level to be serialized.
 * @return A level proto.
 */
proto::Level SerializeLevel(LevelInterface& level);

} // End namespace frame::proto.
