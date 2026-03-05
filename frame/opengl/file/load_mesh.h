#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "frame/level_interface.h"
#include "frame/node_mesh.h"
#include "frame/mesh_interface.h"

namespace frame::opengl::file
{

/**
 * @brief Load meshes from file.
 * @param level: The level in which you want to load the mesh.
 * @param file: The file name of the mesh.
 * @param name: The name of the mesh.
 * @param material_name: The material that is used.
 * @return A vector of pairs containing the entity id of the node and the
 *         entity id of the material.
 */
std::vector<std::pair<EntityId, EntityId>> LoadMeshesFromFile(
    LevelInterface& level,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name = "",
    proto::NodeMesh::AccelerationStructureEnum acceleration_structure_enum =
        proto::NodeMesh::NO_ACCELERATION,
    EntityId forced_program_id = NullId);

std::vector<std::pair<EntityId, EntityId>> LoadMeshesFromFile(
    LevelInterface& level,
    std::filesystem::path file,
    const std::string& name,
    const std::string& material_name,
    EntityId forced_program_id = NullId);

} // namespace frame::opengl::file




