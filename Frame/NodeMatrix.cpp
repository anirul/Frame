#include "NodeMatrix.h"
#include <stdexcept>

namespace frame {

	glm::mat4 NodeMatrix::GetLocalModel(const double dt) const
	{
		if (!func_("root")) 
		{
			throw std::runtime_error(
				"Should initiate NodeInterface correctly!");
		}
		if (!GetParentName().empty())
		{
			auto parent_node = func_(GetParentName());
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
