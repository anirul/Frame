#pragma once

#include <memory>
#include <utility>

#include "frame/window_interface.h"

namespace frame::vulkan {

	/**
	 * @brief Create an instance of the window in SDL using OpenGL.
	 * @param size: Window size.
	 * @return A unique pointer to the window object.
	 */
	std::unique_ptr<WindowInterface> CreateSDL2VulkanWindow(glm::uvec2 size);
	/*
	 * @brief Create a non window using OpenGL (mostly used for testing).
	 * @param size: Size of the output image.
	 * @return A unique pointer to a fake window object.
	 */
	std::unique_ptr<WindowInterface> CreateSDL2VulkanNone(glm::uvec2 size);

}  // End namespace frame::vulkan.
