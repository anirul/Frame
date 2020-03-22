#include "Camera.h"

namespace sgl {

	const sgl::matrix Camera::LookAt() const
	{
		sgl::vector3 f = (to_ - pos_).Normalize();
		sgl::vector3 u = (up_ - (f - up_)).Normalize();
		sgl::vector3 s = (f % u).Normalize();
		u = (s % f).Normalize();

		sgl::matrix4 result;
		result(0, 0) = s.x;
		result(1, 0) = s.y;
		result(2, 0) = s.z;
		result(0, 1) = u.x;
		result(1, 1) = u.y;
		result(2, 1) = u.z;
		result(0, 2) = f.x;
		result(1, 2) = f.y;
		result(2, 2) = f.z;
		result(3, 0) = pos_.x;
		result(3, 1) = pos_.y;
		result(3, 2) = pos_.z;
		return result;
	}

	const sgl::vector Camera::Direction() const
	{
		sgl::vector to = { to_.x, to_.y, to_.z, 0 };
		sgl::vector pos = { pos_.x, pos_.y, pos_.z, 0 };
		return (to - pos).Normalize();
	}

	const sgl::vector Camera::Position() const
	{
		return { pos_.x, pos_.y, pos_.z, 0 };
	}

}	// End namespace sgl.
