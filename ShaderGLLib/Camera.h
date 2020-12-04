#pragma once

#include <utility>
#include <glm/glm.hpp>

namespace sgl {

	class Camera {
	public:
		Camera(
			const glm::vec3 position = { 0.f, 0.f, 0.f },
			const glm::vec3 front = { 0.f, 0.f, -1.f },
			const glm::vec3 up = { 0.f, 1.f, 0.f },
			const float fov_degrees = 65.0f);

	public:
		const glm::mat4 ComputeView() const;
		const glm::mat4 ComputeProjection(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const float near = .1f,
			const float far = 100.f) const;
		void SetFront(const glm::vec3 vec);
		void SetPosition(const glm::vec3 vec);
		void SetUp(const glm::vec3 vec);

	public:
		void SetFovRadians(const float fov) { fov_rad_ = fov; }
		void SetFovDegrees(const float fov) { fov_rad_ = glm::radians(fov); }
		const glm::vec3 GetFront() const { return front_; }
		const glm::vec3 GetPosition() const { return position_; }
		const glm::vec3 GetRight() const { return right_; }
		const glm::vec3 GetUp() const { return up_; }
		const float GetFovRadians() const { return fov_rad_; }
		const float GetFovDegrees() const { return glm::degrees(fov_rad_); }

	protected:
		void UpdateCameraVectors();
		glm::vec3 position_ = { 0, 0, 0 };
		glm::vec3 front_ = { 0, 0, -1 };
		glm::vec3 up_ = { 0, 1, 0 };
		glm::vec3 right_ = { 1, 0, 0 };
		float yaw_ = -90.0f;
		float pitch_ = 0.0f;
		float fov_rad_ = glm::radians(65.0f);
	};

}	// End namespace sgl.
