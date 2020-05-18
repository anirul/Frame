#pragma once

#include <memory>
#include <functional>
#include "../ShaderGLLib/Device.h"

namespace sgl {

	// Interface to a window.
	struct Window
	{
		virtual void Run() = 0;
		virtual void SetDraw(
			std::function<void(
				const double, 
				std::shared_ptr<Texture>&)> draw_func) = 0;
		virtual void SetUniqueDevice(const std::shared_ptr<Device>& device) = 0;
		virtual std::shared_ptr<Device> GetUniqueDevice() = 0;
		virtual std::pair<std::uint32_t, std::uint32_t> GetSize() const = 0;
		virtual void* GetWindowContext() const = 0;
	};

	// Create an instance of the window in SDL using OpenGL.
	std::shared_ptr<Window> CreateSDLOpenGL(
		std::pair<std::uint32_t, std::uint32_t> size);

} // End namespace sgl.
