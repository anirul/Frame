#pragma once

#include <memory>
#include "Frame/Error.h"
#include "Frame/LevelInterface.h"
#include "Frame/ProgramInterface.h"
#include "Frame/RendererInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/UniformInterface.h"
#include "Frame/OpenGL/FrameBuffer.h"
#include "Frame/OpenGL/RenderBuffer.h"

namespace frame::opengl {

	class Renderer : public RendererInterface 
	{
	public:
		// This will also startup the frame and rendering buffer.
		Renderer(
			LevelInterface* level,
			std::pair<std::uint32_t, std::uint32_t> size);

	public:
		// Set the default matrices.
		void SetProjection(glm::mat4 projection) override
		{ 
			projection_ = std::move(projection); 
		}
		void SetView(glm::mat4 view) override { view_ = std::move(view); }
		// Can be changed by render mesh!
		void SetModel(glm::mat4 model) override { model_ = std::move(model); }
		// Set the cubemap target (used in the render mesh method).
		void SetCubeMapTarget(frame::proto::TextureFrame texture_frame) override
		{
			texture_frame_ = texture_frame;
		}

	public:
		// Render to a mesh at a dt time.
		void RenderMesh(
			StaticMeshInterface* static_mesh,
			MaterialInterface* material = nullptr,
			glm::mat4 model_mat = glm::mat4(1.0f),
			double dt = 0.0) override;
		void RenderNode(
			EntityId node_id, 
			EntityId material_id = NullId,
			double dt = 0.0) override;
		void RenderChildren(EntityId node_id, double dt = 0.0) override;
		void RenderFromRootNode(double dt = 0.0) override;
		// Display the default texture to the screen.
		void Display(double dt = 0.0) override;
		void SetDepthTest(bool enable) override;

	private:
		// Level shared_ptr.
		LevelInterface* level_ = nullptr;
		Logger& logger_ = Logger::GetInstance();
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
		// Texture frame (used in render mesh).
		frame::proto::TextureFrame texture_frame_;
	};

} // End namespace frame::opengl.
