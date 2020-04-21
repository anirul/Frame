#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>

Application::Application(const std::shared_ptr<sgl::Window>& window) :
	window_(window) {}

bool Application::Startup()
{
	auto device = window_->CreateDevice();
	device->Startup();
	auto texture = std::make_shared<sgl::TextureCubeMap>(
		"../Asset/CubeMap/Shiodome.hdr",
		sgl::PixelElementSize::FLOAT,
		sgl::PixelStructure::RGB);
	auto irradiance = sgl::CreateIrradianceCubeMap(
		texture, 
		{ 32, 32 }, 
		sgl::PixelElementSize::FLOAT,
		sgl::PixelStructure::RGB);
	auto apple_mesh = CreateAppleMesh(device, irradiance);
	auto cube_map_mesh = CreateCubeMapMesh(device, texture);

	// Comment out if you want to see the errors.
	// window_->Startup();

	// Pack it into a Scene object.
	sgl::SceneTree scene_tree{};
	{
		auto scene_root = std::make_shared<sgl::SceneMatrix>(
			[](const double dt) -> glm::mat4 
		{
			glm::mat4 r_y(1.0f);
			r_y[1][1] = -1.f;
			const auto dtf = static_cast<float>(dt);
			r_y = glm::rotate(r_y, dtf * -.2f, glm::vec3(0.0f, 1.0f, 0.0f));
			return r_y;
		});
		scene_tree.AddNode(scene_root);
		scene_tree.AddNode(
			std::make_shared<sgl::SceneMesh>(cube_map_mesh),
			scene_root);
		auto scene_matrix = std::make_shared<sgl::SceneMatrix>(
			[](const double dt) -> glm::mat4
		{
			glm::mat4 r_x(1.0f);
			glm::mat4 r_y(1.0f);
			glm::mat4 r_z(1.0f);
			const auto dtf = static_cast<float>(dt);
			r_x = glm::rotate(r_x, dtf * .1f, glm::vec3(1.0f, 0.0f, 0.0f));
			r_y = glm::rotate(r_y, dtf * .0f, glm::vec3(0.0f, 1.0f, 0.0f));
			r_z = glm::rotate(r_z, dtf * .2f, glm::vec3(0.0f, 0.0f, 1.0f));
			return r_x * r_y * r_z;
		});
		scene_tree.AddNode(scene_matrix, scene_root);
		scene_tree.AddNode(
			std::make_shared<sgl::SceneMesh>(apple_mesh),
			scene_matrix);
	}

	device->SetSceneTree(scene_tree);
	return true;
}

void Application::Run()
{
	window_->Run();
}

std::shared_ptr<sgl::Mesh> Application::CreateAppleMesh(
	const std::shared_ptr<sgl::Device>& device,
	const std::shared_ptr<sgl::TextureCubeMap>& irradiance) const
{
	// Create the physically based rendering program.
	auto pbr_program = sgl::CreatePhysicallyBasedRenderingProgram(
		device->GetProjection(),
		device->GetView(),
		device->GetModel());

	// Set the camera position
	pbr_program->UniformVector3(
		"camera_position",
		device->GetCamera().GetPosition());

	// Create lights.
	sgl::LightManager light_manager{};
	const float light_value = 300.f;
	const glm::vec3 light_vec(light_value, light_value, light_value);
	light_manager.AddLight(sgl::Light({ 10.f, 10.f, 10.f }, light_vec));
	light_manager.AddLight(sgl::Light({ 10.f, -10.f, 10.f }, light_vec));
	light_manager.AddLight(sgl::Light({ -10.f, 10.f, 10.f }, light_vec));
	light_manager.AddLight(sgl::Light({ -10.f, -10.f, 10.f }, light_vec));
	light_manager.RegisterToProgram(pbr_program);
	device->SetLightManager(light_manager);

	// Mesh creation.
	auto apple_mesh = std::make_shared<sgl::Mesh>(
		"../Asset/Apple.obj", 
		pbr_program);

	// Create the texture and bind it to the mesh.
	auto texture_manager = device->GetTextureManager();
	texture_manager.AddTexture(
		"Color",
		std::make_shared<sgl::Texture>("../Asset/Apple/Color.jpg"));
	texture_manager.AddTexture(
		"Normal",
		std::make_shared<sgl::Texture>("../Asset/Apple/Normal.jpg"));
	texture_manager.AddTexture(
		"Metallic",
		std::make_shared<sgl::Texture>("../Asset/Apple/Metalness.jpg"));
	texture_manager.AddTexture(
		"Roughness",
		std::make_shared<sgl::Texture>("../Asset/Apple/Roughness.jpg"));
	texture_manager.AddTexture(
		"AmbientOcclusion",
		std::make_shared<sgl::Texture>("../Asset/Apple/AmbientOcclusion.jpg"));
	texture_manager.AddTexture(
		"Irradiance",
		irradiance);
	apple_mesh->SetTextures({ 
		"Color", 
		"Normal", 
		"Metallic", 
		"Roughness", 
		"AmbientOcclusion", 
		"Irradiance" });
	device->SetTextureManager(texture_manager);

	return apple_mesh;
}

std::shared_ptr<sgl::Mesh> Application::CreateCubeMapMesh(
	const std::shared_ptr<sgl::Device>& device,
	const std::shared_ptr<sgl::TextureCubeMap>& texture) const
{
	// Create the cube map program.
	auto cubemap_program = sgl::CreateCubeMapProgram(
		device->GetProjection());

	// Create the mesh for the cube.
	auto cube_mesh = std::make_shared<sgl::Mesh>(
		"../Asset/Cube.obj", 
		cubemap_program);

	// Get the texture manager.
	auto texture_manager = device->GetTextureManager();
	texture_manager.AddTexture(
		"Skybox",
		texture);
	cube_mesh->SetTextures({ "Skybox" });
	device->SetTextureManager(texture_manager);

	// Enable the cleaning of the depth.
	cube_mesh->ClearDepthBuffer(true);
	return cube_mesh;
}
