#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Frame/NodeInterface.h"

namespace frame {

	class NodeMatrix : public NodeInterface
	{
	public:
		NodeMatrix(
			std::function<NodeInterface*(const std::string&)> func, 
			const glm::mat4 matrix) : 
			NodeInterface(func),
			matrix_(matrix) {}

	public:
		glm::mat4 GetLocalModel(const double dt) const override;

	public:
		void SetMatrix(glm::mat4 matrix) { matrix_ = matrix; }

	protected:
		glm::mat4 ComputeLocalRotation(const double dt) const;

	private:
		glm::mat4 matrix_ = glm::mat4(1.f);
	};

} // End namespace frame.
