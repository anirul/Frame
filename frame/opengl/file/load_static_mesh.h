#pragma once

#include <memory>
#include <string>

#include "frame/file/obj.h"
#include "frame/level_interface.h"
#include "frame/node_static_mesh.h"
#include "frame/static_mesh_interface.h"

namespace frame::opengl::file
{

/**
 * @brief Load static meshes from file.
 * @param level: The level in which you want to load the mesh.
 * @param file: The file name of the mesh.
 * @param name: The name of the mesh.
 * @param material_name: The material that is used.
 * @return A vector of pairs containing the entity id of the node and the
 *         entity id of the material.
 */
std::vector<std::pair<EntityId, EntityId>> LoadStaticMeshesFromFile(
    LevelInterface& level,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name = "",
    proto::NodeStaticMesh::AccelerationStructureEnum acceleration_structure_enum =
        proto::NodeStaticMesh::NO_ACCELERATION);

} // namespace frame::opengl::file
