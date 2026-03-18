#pragma once

#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::vulkan::json
{

struct ParseSceneTreeOptions
{
    bool prefer_hardware_raytracing = false;
};

[[nodiscard]] bool ParseSceneTree(
    const frame::proto::SceneTree& proto_scene_tree,
    frame::LevelInterface& level,
    const ParseSceneTreeOptions& options = {});

} // namespace frame::vulkan::json
