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
		// Check if we have a valid quaternion.
		if (quaternion_ != glm::quat(1, 0, 0, 0) &&
			quaternion_ != glm::quat(0, 0, 0, 0))
		{
			// Return the matrix multiplied by the rotation of the quaternion.
			return
				matrix_ *
				glm::toMat4(
					glm::mix(
						glm::quat(1, 0, 0, 0),
						quaternion_,
						static_cast<float>(dt)));
		}
		// Check if euler angler are valid (not 0).
		if (euler_ != glm::vec3(0.f, 0.f, 0.f))
		{
			return
				matrix_ *
				glm::toMat4(glm::quat(euler_ * static_cast<float>(dt)));
		}
		// Nothing to do return the basic matrix.
		return matrix_;
	}

} // End namespace frame.
