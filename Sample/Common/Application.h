#pragma once

#include <memory>
#include <filesystem>

#include "Frame/Window.h"
#include "Sample/Common/PathInterface.h"

class Application
{
public:
	Application(
		std::filesystem::path path,
		std::unique_ptr<frame::WindowInterface>&& window);
	void Startup();
	void Run();

protected:
	std::filesystem::path path_;
	std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};
