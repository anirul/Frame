#pragma once

#include <memory>
#include <string>

#include "frame/file/obj.h"
#include "frame/level_interface.h"
#include "frame/node_static_mesh.h"
#include "frame/static_mesh_interface.h"

namespace frame::opengl::file {

/**
 * @brief Load static meshes from file.
 * @param level: The level in which you want to load the mesh.
 * @param file: The file name of the mesh.
 * @param name: The name of the mesh.
 * @param material_name: The material that is used.
 * @param skip_material_file: Should you skip the material that are in the file?
 * @return The entity id of the meshes in the level (could be more than one in case OBJ file).
 */
std::vector<EntityId> LoadStaticMeshesFromFile(LevelInterface* level,
                                               const std::filesystem::path& file,
                                               const std::string& name,
                                               const std::string& material_name = "");

}  // namespace frame::opengl::file
