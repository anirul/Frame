#pragma once

#include <memory>
#include <string>
#include "Frame/StaticMeshInterface.h"

namespace frame::file {

	std::shared_ptr<StaticMeshInterface> LoadStaticMeshFromFileOpenGL(
		const std::string& file);

} // End namespace frame::file.