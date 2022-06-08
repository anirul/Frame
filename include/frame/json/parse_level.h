#pragma once

#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::proto {

/**
 * @brief Parse a level as a proto to an OpenGL level.
 * @param size: Screen size.
 * @param proto_level: Proto to be parsed.
 * @return A unique pointer to a level interface.
 */
std::optional<std::unique_ptr<LevelInterface>> ParseLevelOpenGL(
    const std::pair<std::int32_t, std::int32_t> size, const proto::Level& proto_level);

}  // End namespace frame::proto.
