#pragma once

#include "frame/backend_internal.h"

#include <glm/glm.hpp>

#include "frame/json/level_data.h"
#include "frame/level_interface.h"

namespace frame::opengl
{

std::unique_ptr<frame::LevelInterface> BuildLevel(
    glm::uvec2 size,
    const frame::json::LevelData& level_data);

}
