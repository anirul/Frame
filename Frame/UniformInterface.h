#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace frame {

	// Get access to essential part of the rendering uniform system.
	struct UniformInterface
	{
		virtual ~UniformInterface() = default;
		virtual glm::vec3 GetCameraPosition() const = 0;
		virtual glm::vec3 GetCameraFront() const = 0;
		virtual glm::vec3 GetCameraRight() const = 0;
		virtual glm::vec3 GetCameraUp() const = 0;
		virtual glm::mat4 GetProjection() const = 0;
		virtual glm::mat4 GetView() const = 0;
		virtual glm::mat4 GetModel() const = 0;
		virtual double GetDeltaTime() const = 0;
	};

} // End namespace frame.
