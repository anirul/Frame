#pragma once

#include <memory>
#include "Frame/LevelInterface.h"
#include "Frame/ProgramInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/Error.h"
#include "Frame/OpenGL/FrameBuffer.h"
#include "Frame/OpenGL/RenderBuffer.h"

namespace frame::opengl {

	class Rendering {
	public:
		// This will also startup the frame and rendering buffer.
		Rendering(
			std::shared_ptr<LevelInterface> level,
			std::pair<std::uint32_t, std::uint32_t> size);

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
		void RenderMesh(
			ProgramInterface* program,
			StaticMeshInterface* static_mesh,
			const double dt = 0.0);
		// Display the default texture to the screen.
		void Display();

	private:
		// Level shared_ptr.
		std::shared_ptr<LevelInterface> level_;
		// Projection / View / Model matrices.
		glm::mat4 projection_ = glm::mat4(1.0f);
		glm::mat4 view_ = glm::mat4(1.0f);
		glm::mat4 model_ = glm::mat4(1.0f);
		// Frame & Render buffers.
		FrameBuffer frame_buffer_{};
		RenderBuffer render_buffer_{};
		const Error& error_ = Error::GetInstance();
		// Display ids.
		EntityId display_program_id_ = 0;
		EntityId display_material_id_ = 0;
	};

} // End namespace frame::opengl.
