#pragma once

#include <memory>
#include "Frame/Window.h"
#include "Frame/OpenGL/Texture.h"

class Application
{
public:
	Application(const std::shared_ptr<frame::WindowInterface> window);
	void Startup();
	void Run();

private:
	std::shared_ptr<frame::WindowInterface> window_ = nullptr;
};