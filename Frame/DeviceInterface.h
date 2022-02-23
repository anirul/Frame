#pragma once

#include <memory>
#include <string>

#include "Frame/CameraInterface.h"
#include "Frame/LevelInterface.h"

namespace frame {

	struct DeviceInterface
	{
        // Clear the Screen.
        virtual void Clear(
			const glm::vec4& color = glm::vec4(.2f, 0.f, .2f, 1.0f)) const = 0;
		// Startup the scene.
		virtual void Startup(std::unique_ptr<LevelInterface>&& level) = 0;
		// Display to the screen
		virtual void Display(double dt = 0.0) = 0;
		// Cleanup the mess.
		virtual void Cleanup() = 0;
		// Get the current level.
		virtual LevelInterface* GetLevel() = 0;
		// Return a device context on the underlying graphic API.
		virtual void* GetDeviceContext() const = 0;
		// Return the name of the underlying API.
		virtual const std::string GetTypeString() const = 0;
	};

} // End namespace frame.