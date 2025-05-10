#pragma once

#include <memory>

#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::json
{

/**
 * @brief Parse a proto to a scene tree (platform independent).
 * @param proto_scene_tree: Proto that contain the scene tree.
 * @param level: A pointer to a level.
 * @return True if success and false if error.
 */
[[nodiscard]] bool ParseSceneTreeFile(
    const proto::SceneTree& proto_scene_tree, LevelInterface& level);

} // End namespace frame::json.
