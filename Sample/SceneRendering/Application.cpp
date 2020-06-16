#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Draw.h"

Application::Application(const std::shared_ptr<sgl::WindowInterface>& window) :
	window_(window) {}

void Application::Startup() 
{
	auto device = window_->GetUniqueDevice();
	device->Startup();
	device->LoadSceneFromObjFile("../Asset/Model/Scene.obj");
	device->AddEnvironment("../Asset/CubeMap/Shiodome.hdr");
}

void Application::Run()
{
	window_->SetDrawInterface(
		std::make_shared<Draw>(window_->GetUniqueDevice()));
	window_->Run();
}

