#pragma once

#include <memory>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Texture.h"

class Application
{
public:
	Application(const std::shared_ptr<sgl::Window>& window);
	bool Startup();
	void Run();

protected:
	std::shared_ptr<sgl::Mesh> CreateCubeMapMesh(
		const std::shared_ptr<sgl::Device>& device,
		const std::shared_ptr<sgl::TextureCubeMap>& texture) const;
	std::shared_ptr<sgl::Mesh> CreateAppleMesh(
		const std::shared_ptr<sgl::Device>& device,
		const std::shared_ptr<sgl::TextureCubeMap>& texture);
	std::shared_ptr<sgl::Mesh> CreateSphereMesh(
		const std::shared_ptr<sgl::Device>& device,
		const std::shared_ptr<sgl::TextureCubeMap>& texture);
	std::vector<std::string> CreateTextures(
		sgl::TextureManager& texture_manager,
		const std::shared_ptr<sgl::TextureCubeMap>& texture,
		const std::string& name) const;
	std::shared_ptr<sgl::Window> window_ = nullptr;
	std::shared_ptr<sgl::Program> pbr_program_ = nullptr;
};