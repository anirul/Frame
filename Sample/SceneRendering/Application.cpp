#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>

Application::Application(const std::shared_ptr<sgl::WindowInterface>& window) :
	window_(window) {}

void Application::Startup() 
{
	auto device = window_->GetUniqueDevice();
	device->Startup();
	device->LoadSceneFromObjFile("../Asset/Model/Scene.obj");	
}

void Application::Run()
{
	window_->Run();
}

