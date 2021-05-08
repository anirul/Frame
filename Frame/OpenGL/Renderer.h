#pragma once

#include <memory>
#include "Frame/Error.h"
#include "Frame/LevelInterface.h"
#include "Frame/ProgramInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/UniformInterface.h"
#include "Frame/OpenGL/FrameBuffer.h"
#include "Frame/OpenGL/RenderBuffer.h"

namespace frame::opengl {

	class Renderer {
	public:
		// This will also startup the frame and rendering buffer.
		Renderer(
			LevelInterface* level,
			UniformInterface* uniform_interface,
			std::pair<std::uint32_t, std::uint32_t> size);

	public:
		// Set the default matrices.
		void SetProjection(glm::mat4 projection)
		{ 
			projection_ = std::move(projection); 
		}
		void SetView(glm::mat4 view) { view_ = std::move(view); }
		// Can be changed by render mesh!
		void SetModel(glm::mat4 model) { model_ = std::move(model); }
		void SetUniformInterface(UniformInterface* uniform_interface)
		{
			uniform_interface_ = uniform_interface;
		}

	public:
		// Render to a mesh at a dt time.
		void RenderMesh(
			StaticMeshInterface* static_mesh,
			const glm::mat4& model_mat = glm::mat4(1.0f),
			const double dt = 0.0);
		void RenderNode(EntityId node_id, const double dt = 0.0);
		void RenderChildren(EntityId node_id, const double dt = 0.0);
		void RenderFromRootNode(const double dt = 0.0);
		// Display the default texture to the screen.
		void Display();

	private:
		// Level shared_ptr.
		LevelInterface* level_ = nullptr;
		// Uniform interface to get the basic model from the scene and camera
		// position and view.
		UniformInterface* uniform_interface_ = nullptr;
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
