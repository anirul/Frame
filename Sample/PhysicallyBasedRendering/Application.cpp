#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Draw.h"

Application::Application(const std::shared_ptr<sgl::WindowInterface>& window) :
	window_(window) {}

void Application::Startup() 
{
	window_->GetUniqueDevice()->Startup();
}

void Application::Run()
{
	window_->SetDrawInterface(
		std::make_shared<Draw>(window_->GetUniqueDevice()));
	window_->Run();
}

