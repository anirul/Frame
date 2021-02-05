#pragma once

#include <memory>
#include "Frame/NodeInterface.h"

namespace frame {

	class NodeStaticMesh : public NodeInterface
	{
	public:
		NodeStaticMesh(std::shared_ptr<StaticMeshInterface> mesh) :
			mesh_(mesh) {}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<StaticMeshInterface>
			GetLocalMesh() const override;

	private:
		std::shared_ptr<StaticMeshInterface> mesh_ = nullptr;
	};

} // End namespace frame.
