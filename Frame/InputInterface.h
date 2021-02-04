#pragma once

#include <glm/glm.hpp>

namespace frame {

	// This is the interface for input.
	struct InputInterface
	{
		// Keyboard interface in case of pressed on a key.
		virtual bool KeyPressed(const char key, const double dt) = 0;
		// Keyboard interface in case of release of a key.
		virtual bool KeyReleased(const char key, const double dt) = 0;
		// Mouse moved in relative and absolute positions.
		virtual bool MouseMoved(
			const glm::vec2 position,
			const glm::vec2 relative,
			const double dt) = 0;
		// Mouse pressed (used for mouse buttons).
		virtual bool MousePressed(const char button, const double dt) = 0;
		// Mouse release (used for mouse buttons).
		virtual bool MouseReleased(const char button, const double dt) = 0;
		// Validate the next frame.
		virtual void NextFrame() = 0;
	};

} // End namespace frame.
