#pragma once

#include <memory>
#include <functional>
#include "../ShaderGLLib/Device.h"

namespace sgl {

	// This is the structure that define what draw has to do.
	struct DrawInterface 
	{
		// Initialize with the size of the out buffer.
		virtual void Startup(
			const std::pair<std::uint32_t, std::uint32_t> size) = 0;
		// This should call the DrawMultiTextures or any draw from the device.
		virtual void RunDraw(const double dt) = 0;
		// Should get the output buffer.
		virtual const std::shared_ptr<Texture>& GetDrawTexture() const = 0;
		// This is there to free optional buffers (if needed).
		virtual void Delete() = 0;
	}; 

	// Interface to a window.
	struct WindowInterface
	{
		virtual void Run() = 0;
		virtual void SetDrawInterface(
			const std::shared_ptr<DrawInterface>& draw_interface) = 0;
		virtual void SetUniqueDevice(const std::shared_ptr<Device>& device) = 0;
		virtual std::shared_ptr<Device> GetUniqueDevice() = 0;
		virtual std::pair<std::uint32_t, std::uint32_t> GetSize() const = 0;
		virtual void* GetWindowContext() const = 0;
	};

	// Create an instance of the window in SDL using OpenGL.
	std::shared_ptr<WindowInterface> CreateSDLOpenGL(
		std::pair<std::uint32_t, std::uint32_t> size);

} // End namespace sgl.
