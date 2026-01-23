#pragma once

#include <glm/glm.hpp>

#include "frame/level_interface.h"
#include "frame/proto/level.pb.h"

namespace frame::opengl
{

std::unique_ptr<frame::LevelInterface> BuildLevelFromProto(
    glm::uvec2 size,
    const frame::proto::Level& proto_level);

}
