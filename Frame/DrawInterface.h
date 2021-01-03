#pragma once

#include <cinttypes>
#include <utility>

namespace frame {

	// Is this really needed?
	// This is the structure that define what draw has to do this is specific
	// to a drawing interface (see DirectX, OpenGL, Metal, etc...).
	struct DrawInterface
	{
		// Initialize with the size of the out buffer.
		virtual void Startup(
			const std::pair<std::uint32_t, std::uint32_t> size) = 0;
		// This should call the DrawMultiTextures or any draw from the device.
		virtual void RunDraw(const double dt) = 0;
	};

} // End namespace frame.