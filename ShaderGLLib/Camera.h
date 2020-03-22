#pragma once

#include "../ShaderGLLib/Vector.h"

namespace sgl {

	class Camera {
	public:
		Camera(
			const sgl::vector3& pos,
			const sgl::vector3& to = { 0.f, 0.f, 1.f },
			const sgl::vector3& up = { 0.f, 1.f, 0.f }) :
			pos_(pos), to_(to), up_(up) {}
		const sgl::matrix LookAt() const;
		const sgl::vector Direction() const;
		const sgl::vector Position() const;

	protected:
		sgl::vector3 pos_ = { 0, 0, 0 };
		sgl::vector3 to_ = { 0, 0, 1 };
		sgl::vector3 up_ = { 0, 1, 0 };
	};

}	// End namespace sgl.
