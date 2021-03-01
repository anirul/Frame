#pragma once

#include <memory>
#include "Frame/LevelInterface.h"
#include "Frame/ProgramInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/Error.h"

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
		// Render to a mesh at a dt time.
		void DisplayMesh(
			LevelInterface* level,
			ProgramInterface* program,
			StaticMeshInterface* static_mesh,
			const double dt = 0.0);

	private:
		glm::mat4 projection_ = glm::mat4(1.0f);
		glm::mat4 view_ = glm::mat4(1.0f);
		glm::mat4 model_ = glm::mat4(1.0f);
		const Error& error_ = Error::GetInstance();
	};

} // End namespace frame::opengl.
