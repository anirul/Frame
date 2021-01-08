#include "Rendering.h"
#include <stdexcept>

namespace frame::opengl {

	void RenderingStaticMesh(
		const glm::mat4 perspective, 
		const glm::mat4 view, 
		const glm::mat4 model, 
		const std::shared_ptr<StaticMeshInterface> mesh, 
		const std::shared_ptr<ProgramInterface> program)
	{
		throw std::runtime_error("Not implemented!");
	}

	void RenderingTexture(
		const glm::mat4 perspective, 
		const glm::mat4 view, 
		const glm::mat4 model, 
		const std::shared_ptr<ProgramInterface> program)
	{
		throw std::runtime_error("Not implemented!");
	}

	void RenderingTextureCubeMap(
		const glm::mat4 perspective, 
		const glm::mat4 view, 
		const glm::mat4 model, 
		const std::shared_ptr<ProgramInterface> program)
	{
		throw std::runtime_error("Not implemented!");
	}

} // End namespace frame::opengl.
