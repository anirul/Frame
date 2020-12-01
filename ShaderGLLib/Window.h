#pragma once

#include <memory>
#include <functional>
#include "../ShaderGLLib/Device.h"

namespace sgl {

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
	};

	// This is the structure that define what draw has to do this is specific
	// to a drawing interface (see DirectX, OpenGL, Metal, etc...).
	struct DrawInterface 
	{
		// Initialize with the size of the out buffer.
		virtual void Startup(
			const std::pair<std::uint32_t, std::uint32_t> size) = 0;
		// This should call the DrawMultiTextures or any draw from the device.
		virtual void RunDraw(const double dt) = 0;
		// Should get the output buffer.
		virtual const std::shared_ptr<Texture> GetDrawTexture() const = 0;
	}; 

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
		virtual void SetUniqueDevice(const std::shared_ptr<Device> device) = 0;
		// Get the current device the one that was assign to this window.
		virtual std::shared_ptr<Device> GetUniqueDevice() = 0;
		// Get the size of the window (useful to make a buffer).
		virtual std::pair<std::uint32_t, std::uint32_t> GetSize() const = 0;
		// Return the context to the window (this is a void* as this can be a
		// OpenGL context or a DirectX device).
		virtual void* GetWindowContext() const = 0;
		// Set the window title (the name of the window).
		virtual void SetWindowTitle(const std::string& title) const = 0;
	};

	// Create an instance of the window in SDL using OpenGL.
	std::shared_ptr<WindowInterface> CreateSDLOpenGL(
		std::pair<std::uint32_t, std::uint32_t> size);

} // End namespace sgl.
