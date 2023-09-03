#pragma once

#include <memory>

#include "frame/device_interface.h"
#include "frame/input_interface.h"

namespace frame::gui {

	std::unique_ptr<InputInterface> CreateInputWasd(
		DeviceInterface& device,
		float move_multiplication,
		float rotation_multiplication);

	std::unique_ptr<InputInterface> CreateInputWasdMouse(
		DeviceInterface& device,
		float move_multiplication,
		float rotation_multiplication,
		float translation_multiplication,
		float wheel_multiplication);

	std::unique_ptr<InputInterface> CreateInputArcball(
		DeviceInterface& device,
		glm::vec3 pivot,
		float move_multiplication,
		float zoom_multiplication);

}  // End namespace frame::gui.
