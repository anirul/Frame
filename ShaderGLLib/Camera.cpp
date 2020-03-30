#include "Camera.h"
#include <cmath>
#if !defined(M_PI)
#define M_PI   3.14159265358979323846264338327950288
#endif
#define DEG2RAD(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define RAD2DEG(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

namespace sgl {

	Camera::Camera(
		const sgl::vector3& position /*= { 0.f, 0.f, 0.f }*/, 
		const sgl::vector3& front /*= { 0.f, 0.f, -1.f }*/, 
		const sgl::vector3& up /*= { 0.f, 1.f, 0.f }*/) :
		position_(position),
		front_(front),
		up_(up)
	{
		UpdateCameraVectors();
	}

	const sgl::matrix Camera::LookAt() const
	{
		sgl::vector3 f = (front_ - position_).Normalize();
		sgl::vector3 u = up_;
		sgl::vector3 s = (f % u.Normalize()).Normalize();
		u = (s % f);

		sgl::matrix4 result;
		result(0, 0) = s.x;
		result(1, 0) = s.y;
		result(2, 0) = s.z;
		result(0, 1) = u.x;
		result(1, 1) = u.y;
		result(2, 1) = u.z;
		result(0, 2) = -f.x;
		result(1, 2) = -f.y;
		result(2, 2) = -f.z;
		result(3, 0) = - (s * position_);
		result(3, 1) = - (u * position_);
		result(3, 2) = f * position_;
		return result;
	}

	void Camera::UpdateCameraVectors()
	{
		sgl::vector3 front;
		front.x = 
			static_cast<float>(
				std::cos(DEG2RAD(yaw_)) * std::cos(DEG2RAD(pitch_)));
		front.y = static_cast<float>(std::sin(DEG2RAD(pitch_)));
		front.z = 
			static_cast<float>(
				std::sin(DEG2RAD(yaw_)) * std::cos(DEG2RAD(pitch_)));
		front_ = front.Normalize();
		right_ = (front_ % world_up_).Normalize();
		up_ = (right_ % front_).Normalize();
	}

}	// End namespace sgl.
