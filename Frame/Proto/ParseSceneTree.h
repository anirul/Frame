#pragma once

#include <memory>

#include "Frame/LevelInterface.h"
#include "Frame/Proto/Proto.h"

namespace frame::proto {

/**
 * @brief Parse a proto to a scene tree (platform independent).
 * @param proto_scene_tree: Proto that contain the scene tree.
 * @param level: A pointer to a level.
 * @return True if success and false if error.
 */
[[nodiscard]] bool ParseSceneTreeFile(const SceneTree& proto_scene_tree, LevelInterface* level);

}  // End namespace frame::proto.
