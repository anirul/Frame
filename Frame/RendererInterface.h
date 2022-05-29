#pragma once

#include <glm/glm.hpp>
#include "Frame/MaterialInterface.h"
#include "Frame/StaticMeshInterface.h"

namespace frame {

	class RendererInterface
	{
	public:
		virtual ~RendererInterface() = default;
		virtual void SetProjection(glm::mat4 projection) = 0;
		virtual void SetView(glm::mat4 view) = 0;
		virtual void SetModel(glm::mat4 model) = 0;
		virtual void SetCubeMapTarget(
			frame::proto::TextureFrame texture_frame) = 0;
		virtual void RenderMesh(
			StaticMeshInterface* static_mesh,
			MaterialInterface* material = nullptr,
			glm::mat4 model_mat = glm::mat4(1.0f),
			double dt = 0.0) = 0;
		virtual void RenderNode(
			EntityId node_id, 
			EntityId material_id = NullId,
			double dt = 0.0) = 0;
		virtual void RenderChildren(EntityId node_id, double dt = 0.0) = 0;
		virtual void RenderFromRootNode(double dt = 0.0) = 0;
		virtual void Display(double dt = 0.0) = 0;
		virtual void SetDepthTest(bool enable) = 0;
	};

} // End namespace frame.