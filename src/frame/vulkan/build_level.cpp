#include "frame/vulkan/build_level.h"

#include "frame/level.h"
#include "frame/logger.h"

namespace frame::vulkan
{

BuiltLevel BuildLevel(
    Device& /*device*/,
    const frame::json::LevelData& level_data)
{
    BuiltLevel built;
    auto level = std::make_unique<frame::Level>();
    level->SetName(level_data.proto.name());
    level->SetDefaultTextureName(level_data.proto.default_texture_name());
    if (level_data.proto.has_scene_tree())
    {
        level->SetDefaultCameraName(
            level_data.proto.scene_tree().default_camera_name());
        level->SetDefaultRootSceneNodeName(
            level_data.proto.scene_tree().default_root_name());
    }
    built.level = std::move(level);
    return built;
}

} // namespace frame::vulkan
