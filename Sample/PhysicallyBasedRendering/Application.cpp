#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Frame.h"
#include "Render.h"

Application::Application(
	const std::shared_ptr<sgl::WindowInterface>& window, 
	const draw_model_enum draw_model /*= draw_model_enum::SPHERE*/,
	const texture_model_enum texture_model /*= texture_model_enum::METAL*/) :
	window_(window),
	draw_model_(draw_model),
	texture_model_(texture_model) {}

bool Application::Startup()
{
	auto device = window_->GetUniqueDevice();
	device->Startup();

	// Create Environment cube map texture.
	auto texture = std::make_shared<sgl::TextureCubeMap>(
		"../Asset/CubeMap/Hamarikyu.hdr",
		std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
		sgl::PixelElementSize::HALF);

	auto mesh = CreatePhysicallyBasedRenderedMesh(device, texture);

	// Create the back cube map.
	auto cube_map_mesh = CreateCubeMapMesh(device, texture);

	// Pack it into a Scene object.
	sgl::SceneTree scene_tree{};
	{
		auto scene_root = std::make_shared<sgl::SceneMatrix>(glm::mat4(1.0));
		scene_tree.AddNode(scene_root);
		scene_tree.AddNode(
			std::make_shared<sgl::SceneMesh>(cube_map_mesh),
			scene_root);
		auto scene_matrix = std::make_shared<sgl::SceneMatrix>(glm::mat4(1.0));
		scene_tree.AddNode(scene_matrix, scene_root);
		scene_tree.AddNode(
			std::make_shared<sgl::SceneMesh>(mesh),
			scene_matrix);
	}

	device->SetSceneTree(scene_tree);
	return true;
}

void Application::Run()
{
	window_->SetDrawInterface(
		std::make_shared<Draw>(window_->GetUniqueDevice(), pbr_program_));
	window_->Run();
}

std::vector<std::string> Application::CreateTextures(
	sgl::TextureManager& texture_manager,
	const std::shared_ptr<sgl::TextureCubeMap>& texture) const
{
	// Get the name of the texture.
	std::string name = texture_model_texture_map_.at(texture_model_);

	// Add the default texture to the texture manager.
	texture_manager.AddTexture("Environment", texture);

	// Create the Monte-Carlo prefilter.
	auto monte_carlo_prefilter = std::make_shared<sgl::TextureCubeMap>(
		std::make_pair<std::uint32_t, std::uint32_t>(128, 128), 
		sgl::PixelElementSize::HALF);
	sgl::FillProgramMultiTextureCubeMapMipmap(
		std::vector<std::shared_ptr<sgl::Texture>>{ monte_carlo_prefilter },
		texture_manager,
		{ "Environment" },
		sgl::CreateProgram("MonteCarloPrefilter"),
		5,
		[](const int mipmap, const std::shared_ptr<sgl::Program>& program)
		{
			float roughness = static_cast<float>(mipmap) / 4.0f;
			program->UniformFloat("roughness", roughness);
		});
	texture_manager.AddTexture("MonteCarloPrefilter", monte_carlo_prefilter);

	// Create the Irradiance cube map texture.
	auto irradiance = std::make_shared<sgl::TextureCubeMap>(
		std::make_pair<std::uint32_t, std::uint32_t>(32, 32),
		sgl::PixelElementSize::HALF);
	sgl::FillProgramMultiTextureCubeMap(
		std::vector<std::shared_ptr<sgl::Texture>>{ irradiance },
		texture_manager,
		{ "Environment" },
		sgl::CreateProgram("IrradianceCubeMap"));
	texture_manager.AddTexture("Irradiance", irradiance);

	// Create the LUT BRDF.
	auto integrate_brdf = std::make_shared<sgl::Texture>(
		std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
		sgl::PixelElementSize::HALF);
	sgl::FillProgramMultiTexture(
		std::vector<std::shared_ptr<sgl::Texture>>{ integrate_brdf },
		texture_manager,
		{},
		sgl::CreateProgram("IntegrateBRDF"));
	texture_manager.AddTexture("IntegrateBRDF", integrate_brdf);

	// Create the texture and bind it to the mesh.
	texture_manager.AddTexture(
		"Color",
		std::make_shared<sgl::Texture>("../Asset/" + name + "/Color.jpg"));
	texture_manager.AddTexture(
		"Normal",
		std::make_shared<sgl::Texture>("../Asset/" + name + "/Normal.jpg"));
	texture_manager.AddTexture(
		"Metallic",
		std::make_shared<sgl::Texture>("../Asset/" + name + "/Metalness.jpg"));
	texture_manager.AddTexture(
		"Roughness",
		std::make_shared<sgl::Texture>("../Asset/" + name + "/Roughness.jpg"));
	texture_manager.AddTexture(
		"AmbientOcclusion",
		std::make_shared<sgl::Texture>(
			"../Asset/" + name + "/AmbientOcclusion.jpg"));

	return 
	{ 
		"Color",
		"Normal",
		"Metallic",
		"Roughness",
		"AmbientOcclusion",
		"MonteCarloPrefilter",
		"Irradiance",
		"IntegrateBRDF" 
	};
}

std::shared_ptr<sgl::Mesh> Application::CreatePhysicallyBasedRenderedMesh(
	const std::shared_ptr<sgl::Device>& device,
	const std::shared_ptr<sgl::TextureCubeMap>& texture)
{
	// Create the physically based rendering program.
	pbr_program_ = sgl::CreateProgram("PhysicallyBasedRendering");
	pbr_program_->UniformMatrix("projection", device->GetProjection());
	pbr_program_->UniformMatrix("view", device->GetView());
	pbr_program_->UniformMatrix("model", device->GetModel());

	// Create lights.
	sgl::LightManager light_manager{};
	const float light_value = 300.f;
	const glm::vec3 light_vec(light_value, light_value, light_value);
	light_manager.AddLight(sgl::Light({ 10.f, 10.f, 10.f }, light_vec));
	light_manager.AddLight(sgl::Light({ 10.f, -10.f, 10.f }, light_vec));
	light_manager.AddLight(sgl::Light({ -10.f, 10.f, 10.f }, light_vec));
	light_manager.AddLight(sgl::Light({ -10.f, -10.f, 10.f }, light_vec));
	light_manager.RegisterToProgram(pbr_program_);
	device->SetLightManager(light_manager);

	// Mesh creation.
	auto mesh = std::make_shared<sgl::Mesh>(
		"../Asset/Model/" + draw_model_shape_map_.at(draw_model_) + ".obj",
		pbr_program_);

	// Get the texture manager.
	auto texture_manager = device->GetTextureManager();
	
	// Set the texture to be used in the shader.
	mesh->SetTextures(CreateTextures(texture_manager, texture));
	device->SetTextureManager(texture_manager);

	return mesh;
}

std::shared_ptr<sgl::Mesh> Application::CreateCubeMapMesh(
	const std::shared_ptr<sgl::Device>& device,
	const std::shared_ptr<sgl::TextureCubeMap>& texture) const
{
	// Create the cube map program.
	auto cubemap_program = sgl::CreateProgram("CubeMapHighDynamicRange");
	cubemap_program->UniformMatrix("projection", device->GetProjection());

	// Create the mesh for the cube.
	auto cube_mesh = sgl::CreateCubeMesh(cubemap_program);

	// Get the texture manager.
	auto texture_manager = device->GetTextureManager();
	texture_manager.AddTexture("Skybox", texture);
	cube_mesh->SetTextures({ "Skybox" });
	device->SetTextureManager(texture_manager);

	// Enable the cleaning of the depth.
	cube_mesh->ClearDepthBuffer(true);
	return cube_mesh;
}
