#include "Application.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Draw.h"

Application::Application(
	std::filesystem::path path,
	std::unique_ptr<frame::WindowInterface>&& window) :
	path_(path),
	window_(std::move(window)) {}

void Application::Startup()
{
	if (window_)
	{
		window_->SetDrawInterface(
			std::make_unique<Draw>(
				window_->GetSize(),
				path_,
				window_->GetUniqueDevice()));
	}
	else
	{
		throw std::runtime_error("[nullptr] is not a window.");
	}
}

void Application::Run()
{
	window_->Run();
}
