#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace frame {

	struct CameraInterface
	{
		virtual const glm::mat4 ComputeView() const = 0;
		virtual const glm::mat4 ComputeProjection(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const float near = .1f,
			const float far = 100.f) const = 0;
		virtual void SetFront(const glm::vec3 vec) = 0;
		virtual void SetPosition(const glm::vec3 vec) = 0;
		virtual void SetUp(const glm::vec3 vec) = 0;
		virtual void SetFovRadians(const float fov) = 0;
		virtual void SetFovDegrees(const float fov) = 0;
		virtual void SetAspectRatio(const float aspect_ratio) = 0;
		virtual void SetNearClip(const float near) = 0;
		virtual void SetFarClip(const float far) = 0;
		virtual const glm::vec3 GetFront() const = 0;
		virtual const glm::vec3 GetPosition() const = 0;
		virtual const glm::vec3 GetRight() const = 0;
		virtual const glm::vec3 GetUp() const = 0;
		virtual const float GetFovRadians() const = 0;
		virtual const float GetFovDegrees() const = 0;
		virtual const float GetAspectRatio() const = 0;
		virtual const float GetNearClip() const = 0;
		virtual const float GetFarClip() const = 0;
	};

} // End namespace frame.
