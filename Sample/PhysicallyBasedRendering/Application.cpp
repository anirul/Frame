#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>

const bool draw_apple = false;

Application::Application(const std::shared_ptr<sgl::Window>& window) :
	window_(window) {}

bool Application::Startup()
{
	auto device = window_->GetUniqueDevice();
	device->Startup();
	// Comment out if you want to see the errors.
	// window_->Startup();

	// Create Environment cube map texture.
	auto texture = std::make_shared<sgl::TextureCubeMap>(
		"../Asset/CubeMap/Shiodome.hdr",
		std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
		sgl::PixelElementSize::FLOAT,
		sgl::PixelStructure::RGB);

	std::shared_ptr<sgl::Mesh> apple_mesh = nullptr;
	std::shared_ptr<sgl::Mesh> sphere_mesh = nullptr;

	if (draw_apple)
	{
		// Create an apple mesh.
		apple_mesh = CreateAppleMesh(device, texture);
	}
	else
	{
		// Create a sphere mesh.
		sphere_mesh = CreateSphereMesh(device, texture);
	}

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
		if (draw_apple)
		{
			scene_tree.AddNode(
				std::make_shared<sgl::SceneMesh>(apple_mesh),
				scene_matrix);
		}
		else
		{
			scene_tree.AddNode(
				std::make_shared<sgl::SceneMesh>(sphere_mesh),
				scene_matrix);
		}
	}

	device->SetSceneTree(scene_tree);
	return true;
}

void Application::Run()
{
	window_->SetDraw([this](const double dt)
	{
		// Update the camera.
		float dtf = static_cast<float>(dt);
		auto device = window_->GetUniqueDevice();
		glm::vec4 position = { 0.f, 0.f, 2.f, 1.f };
		glm::mat4 rot_y(1.0f);
		rot_y = glm::rotate(rot_y, dtf * -.1f, glm::vec3(0.f, 1.f, 0.f));
		sgl::Camera cam(glm::vec3(position * rot_y), { 0.f, 0.f, 0.f });
		device->SetCamera(cam);
		if (pbr_program_)
		{
			// Don't forget to use before setting any uniform.
			pbr_program_->Use();
			pbr_program_->UniformVector3(
				"camera_position",
				device->GetCamera().GetPosition());
		}
	});
	window_->Run();
}

std::vector<std::string> Application::CreateTextures(
	sgl::TextureManager& texture_manager,
	const std::shared_ptr<sgl::TextureCubeMap>& texture,
	const std::string& name) const
{
	// Add the default texture to the texture manager.
	texture_manager.AddTexture("Environment", texture);

	// Create the Monte-Carlo prefilter.
	auto monte_carlo_prefilter = sgl::CreateProgramTextureCubeMapMipmap(
		texture_manager,
		{ "Environment" },
		sgl::CreateProgram("MonteCarloPrefilter"),
		{ 128, 128 },
		5,
		[](const int mipmap, const std::shared_ptr<sgl::Program>& program)
		{
			float roughness = static_cast<float>(mipmap) / 4.0f;
			program->UniformFloat("roughness", roughness);
		},
		sgl::PixelElementSize::FLOAT,
		sgl::PixelStructure::RGB);
	texture_manager.AddTexture("MonteCarloPrefilter", monte_carlo_prefilter);
	// Create the Irradiance cube map texture.
	auto irradiance = sgl::CreateProgramTextureCubeMap(
		texture_manager,
		{ "Environment" },
		sgl::CreateProgram("IrradianceCubeMap"),
		{ 32, 32 },
		sgl::PixelElementSize::FLOAT,
		sgl::PixelStructure::RGB);
	texture_manager.AddTexture("Irradiance", irradiance);
	// Create the LUT BRDF.
	auto integrate_brdf = sgl::CreateProgramTexture(
		texture_manager,
		{},
		sgl::CreateProgram("IntegrateBRDF"),
		{ 512, 512 },
		sgl::PixelElementSize::FLOAT,
		sgl::PixelStructure::RGB);
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

std::shared_ptr<sgl::Mesh> Application::CreateSphereMesh(
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
	auto sphere_mesh = std::make_shared<sgl::Mesh>(
		"../Asset/Model/Sphere.obj",
		pbr_program_);

	// Get the texture manager.
	auto texture_manager = device->GetTextureManager();
	
	// Set the texture to be used in the shader.
	sphere_mesh->SetTextures(CreateTextures(texture_manager, texture, "Metal"));
	device->SetTextureManager(texture_manager);

	return sphere_mesh;
}

std::shared_ptr<sgl::Mesh> Application::CreateAppleMesh(
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
	auto apple_mesh = std::make_shared<sgl::Mesh>(
		"../Asset/Model/Apple.obj", 
		pbr_program_);

	// Get the texture manager.
	auto texture_manager = device->GetTextureManager();
	
	// Set the texture to be used in the shader.
	apple_mesh->SetTextures(CreateTextures(texture_manager, texture, "Apple"));
	device->SetTextureManager(texture_manager);

	return apple_mesh;
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
