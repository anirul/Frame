#pragma once

#include <glm/glm.hpp>
#include "../ShaderGLLib/Camera.h"

namespace sgl {

	// Get access to essential part of the rendering uniform system.
	struct UniformInterface
	{
		virtual const Camera GetCamera() const = 0;
		virtual const glm::mat4 GetProjection() const = 0;
		virtual const glm::mat4 GetView() const = 0;
		virtual const glm::mat4 GetModel() const = 0;
	};

}