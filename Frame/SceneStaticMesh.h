#pragma once

#include <memory>
#include "Frame/SceneNodeInterface.h"

namespace frame {

	class SceneStaticMesh : public SceneNodeInterface
	{
	public:
		SceneStaticMesh(std::shared_ptr<StaticMeshInterface> mesh) :
			mesh_(mesh) {}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<StaticMeshInterface>
			GetLocalMesh() const override;

	private:
		std::shared_ptr<StaticMeshInterface> mesh_ = nullptr;
	};

} // End namespace frame.
