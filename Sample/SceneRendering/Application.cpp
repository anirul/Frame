#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Draw.h"
#include "Input.h"

Application::Application(std::unique_ptr<frame::WindowInterface>&& window) :
	window_(std::move(window)) {}

void Application::Startup() 
{
	auto device = window_->GetUniqueDevice();
	// device->Startup();
	throw std::runtime_error("no way to do this yet.");
	// device->LoadSceneFromObjFile("Asset/Model/Scene.obj");
	// device->AddEnvironment("Asset/CubeMap/Shiodome.hdr");
}

void Application::Run()
{
	window_->SetDrawInterface(
		std::make_unique<Draw>(window_->GetUniqueDevice()));
	window_->SetInputInterface(
		std::make_unique<Input>(window_->GetUniqueDevice()));
	window_->Run();
}
