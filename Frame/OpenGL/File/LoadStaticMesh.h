#pragma once

#include <memory>
#include <string>
#include "Frame/File/Obj.h"
#include "Frame/LevelInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/NodeStaticMesh.h"

namespace frame::opengl::file {

	// Load static meshes from file:
	//	- level					: the level in which you want to load the mesh.
	//	- file					: the file name of the mesh.
	//	- name					: the name of the mesh.
	//	- material_name			: the material that is used.
	//	- skip_material_file	: should you skip the material that are in the
	//							  file?
	// Return the entity id of the mesh in the level.
	std::optional<std::vector<EntityId>> LoadStaticMeshesFromFile(
		LevelInterface* level, 
		const std::string& file,
		const std::string& name,
		const std::string& material_name = "",
		bool skip_material_file = false);

} // End namespace frame::file.
