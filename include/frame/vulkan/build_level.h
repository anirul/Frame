#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "frame/json/level_data.h"
#include "frame/vulkan/device.h"

namespace frame::vulkan
{

struct BuiltLevel
{
    std::unique_ptr<frame::LevelInterface> level;
    // TODO: add Vulkan-specific handles/pipelines as they come online.
};

BuiltLevel BuildLevel(
    Device& device,
    const frame::json::LevelData& level_data);

}
