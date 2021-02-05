#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Frame/NodeInterface.h"

namespace frame {

	class NodeMatrix : public NodeInterface
	{
	public:
		NodeMatrix(const glm::mat4 matrix) : matrix_(matrix) {}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;

	protected:
		glm::mat4 ComputeLocalRotation(const double dt) const;

	private:
		glm::mat4 matrix_ = glm::mat4(1.f);
	};

} // End namespace frame.
