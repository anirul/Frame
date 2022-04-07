#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace frame {

	struct CameraInterface
	{
		virtual ~CameraInterface() = default;
		virtual glm::mat4 ComputeView() const = 0;
		virtual glm::mat4 ComputeProjection() const = 0;
		virtual void SetFront(glm::vec3 vec) = 0;
		virtual void SetPosition(glm::vec3 vec) = 0;
		virtual void SetUp(glm::vec3 vec) = 0;
		virtual void SetFovRadians(float fov) = 0;
		virtual void SetFovDegrees(float fov) = 0;
		virtual void SetAspectRatio(float aspect_ratio) = 0;
		virtual void SetNearClip(float near_clip) = 0;
		virtual void SetFarClip(float far_clip) = 0;
		virtual glm::vec3 GetFront() const = 0;
		virtual glm::vec3 GetPosition() const = 0;
		virtual glm::vec3 GetRight() const = 0;
		virtual glm::vec3 GetUp() const = 0;
		virtual float GetFovRadians() const = 0;
		virtual float GetFovDegrees() const = 0;
		virtual float GetAspectRatio() const = 0;
		virtual float GetNearClip() const = 0;
		virtual float GetFarClip() const = 0;
	};

} // End namespace frame.
