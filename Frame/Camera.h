#pragma once

#include <utility>
#include <glm/glm.hpp>

#include "Frame/CameraInterface.h"

namespace frame {

	class Camera : public CameraInterface
	{
	public:
		Camera(
			glm::vec3 position = { 0.f, 0.f, 0.f },
			glm::vec3 front = { 0.f, 0.f, -1.f },
			glm::vec3 up = { 0.f, 1.f, 0.f },
			float fov_degrees = 65.0f,
			float aspect_ratio = 16.0f / 9.0f,
			float near_clip = 0.1f,
			float far_clip = 1000.0f);

	public:
		glm::mat4 ComputeView() const override;
		glm::mat4 ComputeProjection() const override;
		void SetFront(glm::vec3 vec) override;
		void SetPosition(glm::vec3 vec) override;
		void SetUp(glm::vec3 vec) override;

	public:
		void SetFovRadians(float fov) override { fov_rad_ = fov; }
		void SetFovDegrees(float fov) override { fov_rad_ = glm::radians(fov); }
		void SetAspectRatio(float aspect_ratio) override 
		{
			aspect_ratio_ = aspect_ratio;
		}
		void SetNearClip(float near_clip) override { near_clip_ = near_clip; }
		void SetFarClip(float far_clip) override { far_clip_ = far_clip; }
		glm::vec3 GetFront() const override { return front_; }
		glm::vec3 GetPosition() const override { return position_; }
		glm::vec3 GetRight() const override { return right_; }
		glm::vec3 GetUp() const override { return up_; }
		float GetFovRadians() const override { return fov_rad_; }
		float GetFovDegrees() const override { return glm::degrees(fov_rad_); }
		float GetAspectRatio() const override { return aspect_ratio_; }
		float GetNearClip() const override { return near_clip_; }
		float GetFarClip() const override { return far_clip_; }

	protected:
		void UpdateCameraVectors();
		glm::vec3 position_ = { 0, 0, 0 };
		glm::vec3 front_ = { 0, 0, -1 };
		glm::vec3 up_ = { 0, 1, 0 };
		glm::vec3 right_ = { 1, 0, 0 };
		float yaw_ = -90.0f;
		float pitch_ = 0.0f;
		float fov_rad_ = glm::radians(65.0f);
		float aspect_ratio_ = 16.f / 9.f;
		float near_clip_ = 0.1f;
		float far_clip_ = 1000.0f;
	};

}	// End namespace frame.
