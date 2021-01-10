#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Draw.h"

Application::Application(
	const std::shared_ptr<NameInterface>& name,
	const std::shared_ptr<frame::WindowInterface>& window) :
	name_(name),
	window_(window) {}

void Application::Startup()
{
	auto draw = std::make_shared<Draw>(
		window_->GetSize(), 
		name_, 
		window_->GetUniqueDevice());
	window_->SetDrawInterface(draw);
}

void Application::Run()
{
	window_->Run();
}
