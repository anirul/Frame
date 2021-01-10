#pragma once

#include <memory>
#include "Frame/Window.h"
#include "Sample/Common/NameInterface.h"

class Application
{
public:
	Application(
		const std::shared_ptr<NameInterface>& name,
		const std::shared_ptr<frame::WindowInterface>& window);
	void Startup();
	void Run();

protected:
	std::shared_ptr<frame::WindowInterface> window_;
	std::shared_ptr<NameInterface> name_ = nullptr;
};
