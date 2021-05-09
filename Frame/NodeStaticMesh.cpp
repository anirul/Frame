#include "NodeStaticMesh.h"
#include <stdexcept>

namespace frame {

	const glm::mat4 NodeStaticMesh::GetLocalModel(const double dt) const
	{
		if (!GetParentName().empty())
		{
			NodeInterface::Ptr parent_node = func_(GetParentName());
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

	const EntityId NodeStaticMesh::GetLocalMesh() const
	{
		return static_mesh_id_;
	}

} // End namespace frame.
