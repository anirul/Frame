#pragma once

#include <utility>
#include <string>
#include <SDL.h>

namespace SoftwareGL {

	class WindowInterface
	{
	public:
		// Startup of the window (with GL version).
		virtual bool Startup(const std::pair<int, int>& gl_version) = 0;
		// Compute run (called after the event run).
		virtual bool RunCompute(const double delta_time) = 0;
		// Event run will give the event.
		virtual bool RunEvent(const SDL_Event& event) = 0;
		// Cleanup after the window has been closed.
		virtual void Cleanup() = 0;
		// Get size of the window.
		virtual const std::pair<int, int> GetWindowSize() const = 0;
	};

} // End of namespace SoftwareGL.
