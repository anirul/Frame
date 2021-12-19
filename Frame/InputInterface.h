#pragma once

#include <glm/glm.hpp>

namespace frame {

	// This is the interface for input.
	struct InputInterface
	{
		// Keyboard interface in case of pressed on a key.
		virtual bool KeyPressed(char key, double dt) = 0;
		// Keyboard interface in case of release of a key.
		virtual bool KeyReleased(char key, double dt) = 0;
		// Mouse moved in relative and absolute positions.
		virtual bool MouseMoved(
			glm::vec2 position, 
			glm::vec2 relative,
			double dt) = 0;
		// Mouse pressed (used for mouse buttons).
		virtual bool MousePressed(char button, double dt) = 0;
		// Mouse release (used for mouse buttons).
		virtual bool MouseReleased(char button, double dt) = 0;
		// Validate the next frame.
		virtual void NextFrame() = 0;
	};

} // End namespace frame.
