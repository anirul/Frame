#pragma once

#include "../ShaderGLLib/Vector.h"

namespace sgl {

	class Camera {
	public:
		Camera(
			const sgl::vector3& position = { 0.f, 0.f, 0.f },
			const sgl::vector3& front = { 0.f, 0.f, -1.f },
			const sgl::vector3& up = { 0.f, 1.f, 0.f });
		const sgl::matrix LookAt() const;
		const sgl::vector3 Front() const { return front_; }
		const sgl::vector3 Position() const { return position_; }

	protected:
		void UpdateCameraVectors();
		sgl::vector3 position_ = { 0, 0, 0 };
		sgl::vector3 front_ = { 0, 0, -1 };
		sgl::vector3 up_ = { 0, 1, 0 };
		sgl::vector3 right_ = { 1, 0, 0 };
		sgl::vector3 world_up_ = { 0, 1, 0 };
		float yaw_ = -90.0f;
		float pitch_ = 0.0f;
	};

}	// End namespace sgl.
