#pragma once

#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::vulkan::json
{

[[nodiscard]] bool ParseSceneTree(
    const frame::proto::SceneTree& proto_scene_tree,
    frame::LevelInterface& level);

} // namespace frame::vulkan::json
