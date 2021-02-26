#pragma once

#include <memory>
#include "Frame/LevelInterface.h"
#include "Frame/ProgramInterface.h"
#include "Frame/StaticMeshInterface.h"

namespace frame::opengl {

	class Rendering {
	public:
		Rendering() = default;

	public:
		// Set the default matrices.
		void SetProjection(glm::mat4 projection) 
		{ 
			projection_ = std::move(projection); 
		}
		void SetView(glm::mat4 view) { view_ = std::move(view); }
		void SetModel(glm::mat4 model) { model_ = std::move(model); }

	public:
		// Render to a texture at a dt time.
		void DisplayTexture(
			std::weak_ptr<ProgramInterface> program, 
			const double dt = 0.0);
		// Render to a cube map at a dt time.
		void DisplayCubeMap(
			std::weak_ptr<ProgramInterface> program,
			const double dt = 0.0);
		// Render to a mesh at a dt time.
		void DisplayMesh(
			std::weak_ptr<ProgramInterface> program,
			std::weak_ptr<StaticMeshInterface> static_mesh,
			const double dt = 0.0);

	private:
		glm::mat4 projection_ = glm::mat4(1.0f);
		glm::mat4 view_ = glm::mat4(1.0f);
		glm::mat4 model_ = glm::mat4(1.0f);
	};

} // End namespace frame::opengl.
