#include "NodeStaticMesh.h"
#include <stdexcept>
#include <fmt/core.h>

namespace frame {

	const glm::mat4 NodeStaticMesh::GetLocalModel(const double dt) const
	{
		if (!GetParentName().empty())
		{
			NodeInterface::Ptr parent_node = func_(GetParentName());
			if (!parent_node)
			{
				throw std::runtime_error(
					fmt::format(
						"SceneStaticMesh func({}) returned nullptr", 
						GetParentName()));
			}
			return parent_node->GetLocalModel(dt);
		}
		return glm::mat4(1.0f);
	}

} // End namespace frame.
