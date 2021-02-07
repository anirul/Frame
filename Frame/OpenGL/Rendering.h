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
		void SetPerspective(const glm::mat4& perspective) 
		{ 
			perspective_ = perspective; 
		}
		void SetView(const glm::mat4& view) { view_ = view; }
		void SetModel(const glm::mat4& model) { model_ = model; }

	public:
		// Render a level.
		void DisplayLevel(
			const std::shared_ptr<LevelInterface>& level, 
			const double dt);
		// Render to a texture.
		void DisplayTexture(const std::shared_ptr<ProgramInterface>& program);
		// Render to a cube map.
		void DisplayCubeMap(const std::shared_ptr<ProgramInterface>& program);

	protected:
		glm::mat4 perspective_ = glm::mat4(1.0f);
		glm::mat4 view_ = glm::mat4(1.0f);
		glm::mat4 model_ = glm::mat4(1.0f);
	};

} // End namespace frame::opengl.
