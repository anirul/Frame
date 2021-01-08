#pragma once

#include <memory>
#include "Frame/Window.h"
#include "Frame/OpenGL/Texture.h"

class Application
{
public:
	Application(const std::shared_ptr<frame::WindowInterface>& window);
	void Startup();
	void Run();

protected:
	std::shared_ptr<frame::WindowInterface> window_;
};
