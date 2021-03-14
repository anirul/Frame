#pragma once

#include <memory>
#include <string>
#include "Frame/CameraInterface.h"
#include "Frame/LevelInterface.h"

namespace frame {

	struct DeviceInterface
	{
		// Startup the scene.
		virtual void Startup(const std::shared_ptr<LevelInterface> level) = 0;
		// Cleanup the mess.
		virtual void Cleanup() = 0;
		// Display the output texture to the display.
		virtual void Display(const double dt) = 0;
		// Enable/Disable depth test.
		virtual void SetDepthTest(bool enable) = 0;
		// Return a device context on the underlying graphic API.
		virtual void* GetDeviceContext() const = 0;
		// Return the name of the underlying API.
		virtual const std::string GetTypeString() const = 0;
		// Return a pointer to the camera.
		virtual const std::shared_ptr<CameraInterface> GetCamera() const = 0;
	};

} // End namespace frame.