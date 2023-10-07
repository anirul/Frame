#pragma once

#include <memory>
#include <optional>

#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::proto
{

/**
 * @brief Parse material from a proto file to an OpenGL version of material.
 * @param proto_material: A proto that contain a material.
 * @param level: A level interface (to add the material to the level).
 * @return A unique pointer to a material interface.
 */
std::optional<std::unique_ptr<MaterialInterface>> ParseMaterialOpenGL(
    const frame::proto::Material& proto_material, LevelInterface& level);

} // End namespace frame::proto.
