#pragma once

#include <glm/glm.hpp>

namespace sgl {

	class Camera {
	public:
		Camera(
			const glm::vec3& position = { 0.f, 0.f, 0.f },
			const glm::vec3& front = { 0.f, 0.f, -1.f },
			const glm::vec3& up = { 0.f, 1.f, 0.f });

	public:
		const glm::mat4 GetLookAt() const;
		void SetFront(const glm::vec3& vec);
		void SetPosition(const glm::vec3& vec);

	public:
		const glm::vec3 GetFront() const { return front_; }
		const glm::vec3 GetPosition() const { return position_; }

	protected:
		void UpdateCameraVectors();
		glm::vec3 position_ = { 0, 0, 0 };
		glm::vec3 front_ = { 0, 0, -1 };
		glm::vec3 up_ = { 0, 1, 0 };
		glm::vec3 right_ = { 1, 0, 0 };
		glm::vec3 world_up_ = { 0, 1, 0 };
		float yaw_ = -90.0f;
		float pitch_ = 0.0f;
	};

}	// End namespace sgl.
