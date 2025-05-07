#pragma once

#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::proto
{

/**
 * @brief Serialize a scene tree from a level interface.
 * @param level_interface: The level interface from which the SceneTree come
 * from.
 * @return The proto scene tree.
 */
proto::SceneTree SerializeSceneTree(const LevelInterface& level_interface);

} // End namespace frame::proto.
