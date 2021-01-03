#include "Camera.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace frame::opengl {

	Camera::Camera(
		const glm::vec3 position /*= { 0.f, 0.f, 0.f }*/, 
		const glm::vec3 front /*= { 0.f, 0.f, 1.f }*/, 
		const glm::vec3 up /*= { 0.f, 1.f, 0.f }*/,
		const float fov_degrees /*= 65.0f */) :
		position_(position),
		front_(front),
		up_(up),
		fov_rad_(glm::radians(fov_degrees))
	{
		UpdateCameraVectors();
	}

	const glm::mat4 Camera::ComputeView() const
	{
		return glm::lookAt(position_, front_ + position_, up_);
	}

	const glm::mat4 Camera::ComputeProjection(
		const std::pair<std::uint32_t, std::uint32_t> size,
		const float near,
		const float far) const
	{
		const float aspect =
			static_cast<float>(size.first) / static_cast<float>(size.second);
		return glm::perspective(fov_rad_, aspect, near, far);
	}

	void Camera::SetFront(const glm::vec3 vec)
	{
		front_ = vec;
		UpdateCameraVectors();
	}

	void Camera::SetPosition(const glm::vec3 vec)
	{
		position_ = vec;
		UpdateCameraVectors();
	}

	void Camera::SetUp(const glm::vec3 vec)
	{
		up_ = vec;
		UpdateCameraVectors();
	}

	void Camera::UpdateCameraVectors()
	{
		// TODO(anirul): Check field of view for correctness.
		front_ = glm::normalize(front_);
		right_ = glm::normalize(glm::cross(front_, up_));
		up_ = glm::normalize(glm::cross(right_, front_));
	}

}	// End namespace frame::opengl.
