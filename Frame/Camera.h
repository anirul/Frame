#pragma once

#include <utility>
#include <glm/glm.hpp>
#include "Frame/CameraInterface.h"

namespace frame {

	class Camera : public CameraInterface
	{
	public:
		Camera(
			const glm::vec3 position = { 0.f, 0.f, 0.f },
			const glm::vec3 front = { 0.f, 0.f, -1.f },
			const glm::vec3 up = { 0.f, 1.f, 0.f },
			const float fov_degrees = 65.0f,
			const float aspect_ratio = 16.0f / 9.0f,
			const float near_clip = 0.1f,
			const float far_clip = 1000.0f);

	public:
		const glm::mat4 ComputeView() const override;
		const glm::mat4 ComputeProjection(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const float near = .1f,
			const float far = 100.f) const override;
		void SetFront(const glm::vec3 vec) override;
		void SetPosition(const glm::vec3 vec) override;
		void SetUp(const glm::vec3 vec) override;

	public:
		void SetFovRadians(const float fov) override { fov_rad_ = fov; }
		void SetFovDegrees(const float fov) override
		{
			fov_rad_ = glm::radians(fov);
		}
		void SetAspectRatio(const float aspect_ratio) override
		{
			aspect_ratio_ = aspect_ratio;
		}
		void SetNearClip(const float near_clip) override 
		{
			near_clip_ = near_clip; 
		}
		void SetFarClip(const float far_clip) override 
		{ 
			far_clip_ = far_clip;	
		}
		const glm::vec3 GetFront() const override { return front_; }
		const glm::vec3 GetPosition() const override { return position_; }
		const glm::vec3 GetRight() const override { return right_; }
		const glm::vec3 GetUp() const override { return up_; }
		const float GetFovRadians() const override { return fov_rad_; }
		const float GetFovDegrees() const override 
		{ 
			return glm::degrees(fov_rad_); 
		}
		const float GetAspectRatio() const override { return aspect_ratio_; }
		const float GetNearClip() const override { return near_clip_; }
		const float GetFarClip() const override { return far_clip_; }

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
