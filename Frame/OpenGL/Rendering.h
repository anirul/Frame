#pragma once

#include <memory>
#include "Frame/ProgramInterface.h"
#include "Frame/StaticMeshInterface.h"

namespace frame::opengl {

	// Render a mesh with an effect.
	void RenderingStaticMesh(
		const glm::mat4 perspective,
		const glm::mat4 view,
		const glm::mat4 model,
		const std::shared_ptr<StaticMeshInterface> mesh, 
		const std::shared_ptr<ProgramInterface> program);

	// Render a texture with an effect.
	void RenderingTexture(
		const glm::mat4 perspective,
		const glm::mat4 view,
		const glm::mat4 model,
		const std::shared_ptr<ProgramInterface> program);

	// Render a texture 3D with an effect.
	void RenderingTextureCubeMap(
		const glm::mat4 perspective,
		const glm::mat4 view,
		const glm::mat4 model,
		const std::shared_ptr<ProgramInterface> program);

} // End namespace frame::opengl.
