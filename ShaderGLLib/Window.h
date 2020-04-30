#pragma once

#include <memory>
#include <functional>
#include "../ShaderGLLib/Device.h"

namespace sgl {

	// Interface to a window.
	struct Window
	{
		virtual void Startup() = 0;
		virtual void Run() = 0;
		virtual void SetDraw(std::function<void(const double)> draw_func) = 0;
		virtual std::shared_ptr<Device> GetUniqueDevice() = 0;
		virtual std::pair<int, int> GetSize() const = 0;
	};

	// Create an instance of the window.
	std::shared_ptr<Window> CreateSDLOpenGL(std::pair<int, int> size);

} // End namespace sgl.
