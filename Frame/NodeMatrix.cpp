#include "NodeMatrix.h"
#include <stdexcept>

namespace frame {

	const glm::mat4 NodeMatrix::GetLocalModel(const double dt) const
	{
		if (!GetParentName().empty())
		{
			NodeInterface::Ptr parent_node = func_(GetParentName());
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

	glm::mat4 NodeMatrix::ComputeLocalRotation(const double dt) const
	{
		return matrix_;
	}

} // End namespace frame.
