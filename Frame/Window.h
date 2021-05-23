#pragma once

#include <memory>
#include <utility>
#include "Frame/WindowInterface.h"

namespace frame {

	// Create an instance of the window in SDL using OpenGL.
	std::unique_ptr<WindowInterface> CreateSDLOpenGL(
		std::pair<std::uint32_t, std::uint32_t> size);

} // End namespace frame.
