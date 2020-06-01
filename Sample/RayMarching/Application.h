#pragma once

#include <memory>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Texture.h"

class Application
{
public:
	Application(const std::shared_ptr<sgl::WindowInterface>& window);
	void Startup();
	void Run();

protected:
	std::shared_ptr<sgl::Mesh> CreateBillboardMesh() const;
	std::shared_ptr<sgl::WindowInterface> window_;
};