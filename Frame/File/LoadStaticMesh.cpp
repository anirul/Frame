#include "LoadStaticMesh.h"
#include <stdexcept>

namespace frame::file {

	std::shared_ptr<StaticMeshInterface> LoadStaticMeshFromFileOpenGL(
		const std::shared_ptr<LevelInterface> level,
		const std::string& file)
	{
		throw std::runtime_error("Not implemented!");
	}

} // End namespace frame::file.
