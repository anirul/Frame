#include "SceneStaticMesh.h"
#include <stdexcept>

namespace frame {

	const glm::mat4 SceneStaticMesh::GetLocalModel(const double dt) const
	{
		if (!GetParentName().empty())
		{
			SceneNodeInterface::Ptr parent_node = func_(GetParentName());
			if (!parent_node)
			{
				throw std::runtime_error(
					"SceneStaticMesh func(" +
					GetParentName() +
					") returned nullptr");
			}
			return parent_node->GetLocalModel(dt);
		}
		return glm::mat4(1.0f);
	}

	const std::shared_ptr<StaticMeshInterface>
		SceneStaticMesh::GetLocalMesh() const
	{
		return mesh_;
	}

} // End namespace frame.
