#include "Camera.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace sgl {

	Camera::Camera(
		const glm::vec3& position /*= { 0.f, 0.f, 0.f }*/, 
		const glm::vec3& front /*= { 0.f, 0.f, 1.f }*/, 
		const glm::vec3& up /*= { 0.f, 1.f, 0.f }*/) :
		position_(position),
		front_(front),
		up_(up)
	{
		UpdateCameraVectors();
	}

	const glm::mat4 Camera::GetLookAt() const
	{
		return glm::lookAt(position_, front_ + position_, up_);
	}

	void Camera::SetFront(const glm::vec3& vec)
	{
		front_ = vec;
		UpdateCameraVectors();
	}

	void Camera::SetPosition(const glm::vec3& vec)
	{
		position_ = vec;
		UpdateCameraVectors();
	}

	void Camera::UpdateCameraVectors()
	{
		front_ = glm::normalize(front_);
		right_ = glm::normalize(glm::cross(front_, up_));
		up_ = glm::normalize(glm::cross(right_, front_));
	}

}	// End namespace sgl.
