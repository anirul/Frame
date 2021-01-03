#pragma once

#include <memory>
#include "../Frame/LevelInterface.h"

namespace frame {

	struct DeviceInterface
	{
		// Startup the scene.
		virtual void Startup(const std::shared_ptr<LevelInterface> level) = 0;
		// Cleanup the mess.
		virtual void Cleanup() = 0;
		// Display the output texture to the display.
		virtual void Display(const double dt) = 0;
		virtual void* GetDeviceContext() const = 0;
		virtual const std::string GetTypeString() const = 0;
	};

} // End namespace frame.