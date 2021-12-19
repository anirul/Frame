#pragma once

#include <memory>
#include "Frame/Window.h"
#include "Frame/OpenGL/Texture.h"

class Application
{
public:
	Application(std::unique_ptr<frame::WindowInterface>&& window);
	void Startup();
	void Run();

private:
	std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};