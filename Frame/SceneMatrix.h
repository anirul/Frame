#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Frame/SceneNodeInterface.h"

namespace frame {

	class SceneMatrix : public SceneNodeInterface
	{
	public:
		SceneMatrix(const glm::mat4 matrix) : matrix_(matrix) {}
		SceneMatrix(const glm::mat4 matrix, const glm::vec3 euler) :
			matrix_(matrix), euler_(euler) {}
		SceneMatrix(const glm::mat4 matrix, const glm::quat quaternion) :
			matrix_(matrix), quaternion_(quaternion) {}

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;

	protected:
		glm::mat4 ComputeLocalRotation(const double dt) const;

	private:
		glm::mat4 matrix_ = glm::mat4(1.f);
		glm::vec3 euler_ = { 0.f, 0.f, 0.f };
		glm::quat quaternion_ = { 1.f, 0.f, 0.f, 0.f };
	};

} // End namespace frame.
