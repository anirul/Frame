#pragma once

#include <memory>
#include <string>
#include <utility>
#include "Frame/Frame.h"

namespace frame {

	// Interface to a window this is specific to a platform (see Windows, SDL, 
	// Linux, OSX, iOS, etc...).
	struct WindowInterface
	{
		// Run the windows interface this will take the current thread.
		virtual void Run() = 0;
		// Set the drawing interface (see above).
		virtual void SetDrawInterface(
			const std::shared_ptr<DrawInterface> draw_interface) = 0;
		// Set the input interface (see above).
		virtual void SetInputInterface(
			const std::shared_ptr<InputInterface> input_interface) = 0;
		// Get the unique device (this is suppose to be variable to the one you
		// are using see : DirectX, OpenGL, etc...).
		virtual void SetUniqueDevice(
			const std::shared_ptr<DeviceInterface> device) = 0;
		// Get the current device the one that was assign to this window.
		virtual std::shared_ptr<DeviceInterface> GetUniqueDevice() = 0;
		// Get the size of the window (useful to make a buffer).
		virtual std::pair<std::uint32_t, std::uint32_t> GetSize() const = 0;
		// Return the context to the window (this is a void* as this can be a
		// Windows HWND, a Linux window or ?).
		virtual void* GetWindowContext() const = 0;
		// Set the window title (the name of the window).
		virtual void SetWindowTitle(const std::string& title) const = 0;
	};

} // End namespace frame.
