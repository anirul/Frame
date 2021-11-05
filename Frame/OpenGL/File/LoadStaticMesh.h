#pragma once

#include <memory>
#include <string>
#include "Frame/File/Obj.h"
#include "Frame/LevelInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/NodeStaticMesh.h"

namespace frame::opengl::file {

	std::optional<std::vector<EntityId>> LoadStaticMeshesFromFile(
		LevelInterface* level, 
		const std::string& file,
		const std::string& name);

} // End namespace frame::file.
