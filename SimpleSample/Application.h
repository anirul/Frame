#pragma once

#include <memory>
#include "../ShaderGLLib/Window.h"

class Application
{
public:
	Application(const std::shared_ptr<sgl::Window>& window);
	bool Startup();
	void Run();

protected:
	std::shared_ptr<sgl::Window> window_;
};