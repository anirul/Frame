#include "SceneMatrix.h"
#include <stdexcept>

namespace frame {

	const glm::mat4 SceneMatrix::GetLocalModel(const double dt) const
	{
		if (!GetParentName().empty())
		{
			SceneNodeInterface::Ptr parent_node = func_(GetParentName());
			if (!parent_node)
			{
				throw std::runtime_error(
					"SceneMatrix func(" +
					GetParentName() +
					") returned nullptr");
			}
			return
				parent_node->GetLocalModel(dt) *
				ComputeLocalRotation(dt);
		}
		return ComputeLocalRotation(dt);
	}

	glm::mat4 SceneMatrix::ComputeLocalRotation(const double dt) const
	{
		return matrix_;
	}

} // End namespace frame.
