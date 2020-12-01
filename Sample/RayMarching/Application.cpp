#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Draw.h"

Application::Application(const std::shared_ptr<sgl::WindowInterface> window) :
	window_(window) {}

void Application::Startup() {}

void Application::Run()
{
	auto draw = std::make_shared<Draw>(window_->GetUniqueDevice());
	window_->SetDrawInterface(draw);
	window_->Run();
}

