#include "Rendering.h"
#include <stdexcept>

namespace frame::opengl {

	void Rendering::DisplayLevel(
		const std::shared_ptr<LevelInterface>& level, 
		const double dt)
	{
		throw std::runtime_error("Not implemented!");
	}

	void Rendering::DisplayTexture(
		const std::shared_ptr<ProgramInterface>& program)
	{
		throw std::runtime_error("Not implemented!");
	}

	void Rendering::DisplayCubeMap(
		const std::shared_ptr<ProgramInterface>& program)
	{
		throw std::runtime_error("Not implemented!");
	}

} // End namespace frame::opengl.
