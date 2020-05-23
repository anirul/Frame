#pragma once

#include <memory>
#include <functional>
#include "../ShaderGLLib/Device.h"

namespace sgl {

	// This is the structure that define what draw has to do.
	struct DrawInterface 
	{
		virtual void Initialize(
			const std::pair<std::uint32_t, std::uint32_t> size) = 0;
		virtual void Run(const double dt) = 0;
		virtual const std::vector<std::shared_ptr<Texture>>& GetTextures() = 0;
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
