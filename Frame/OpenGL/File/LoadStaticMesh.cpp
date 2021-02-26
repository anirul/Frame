#include "Frame/OpenGL/File/LoadStaticMesh.h"
#include <stdexcept>

namespace frame::opengl::file {

	std::shared_ptr<StaticMeshInterface> LoadStaticMeshFromFile(
		const std::shared_ptr<LevelInterface> level,
		const std::string& file)
	{
		throw std::runtime_error("Not implemented!");
	}

} // End namespace frame::file.
