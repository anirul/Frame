#include "NodeCamera.h"
#include <stdexcept>

namespace frame {

	glm::mat4 NodeCamera::GetLocalModel(const double dt) const
	{
		if (!GetParentName().empty())
		{
			auto parent_node = func_(GetParentName());
			if (!parent_node)
			{
				throw std::runtime_error(
					"SceneCamera func(" +
					GetParentName() +
					") returned nullptr");
			}
			return parent_node->GetLocalModel(dt);
		}
		return glm::mat4(1.0f);
	}

} // End namespace frame.
