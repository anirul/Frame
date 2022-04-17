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
		void AddClearFlags(std::uint32_t value) { clear_flags |= value; }
		std::uint32_t GetClearFlags() const { return clear_flags; }

	protected:
		glm::mat4 ComputeLocalRotation(const double dt) const;

	private:
		glm::mat4 matrix_ = glm::mat4(1.f);
		std::uint32_t clear_flags = 0;
	};

} // End namespace frame.
