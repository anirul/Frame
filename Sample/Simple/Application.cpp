#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>

Application::Application(const std::shared_ptr<sgl::Window>& window) :
	window_(window) {}

bool Application::Startup()
{
	auto device = window_->GetUniqueDevice();
	device->Startup();
	// TODO(anirul): Correct this mess in the CreateCubeMesh!
	auto texture = std::make_shared<sgl::TextureCubeMap>(
		std::array<std::string, 6>{ 
			"../Asset/CubeMap/PositiveX.png", "../Asset/CubeMap/NegativeX.png", 
			"../Asset/CubeMap/NegativeY.png", "../Asset/CubeMap/PositiveY.png",
			"../Asset/CubeMap/PositiveZ.png", "../Asset/CubeMap/NegativeZ.png",
		});
	auto apple_mesh = CreateAppleMesh(device);
	auto cube_map_mesh = CreateCubeMapMesh(device, texture);

	// Pack it into a Scene object.
	sgl::SceneTree scene_tree{};
	{
		auto scene_root = std::make_shared<sgl::SceneMatrix>(glm::mat4(
			1.f, 0.f, 0.f, 0.f,
			0.f, -1.f, 0.f, 0.f, // Why???
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f));
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
	window_->SetDraw([this](
		const double dt,
		std::shared_ptr<sgl::Texture>& texture)
	{
		float dtf = static_cast<float>(dt);
		auto device = window_->GetUniqueDevice();
		glm::vec4 position = { 0.f, 0.f, 2.f, 1.f };
		glm::mat4 rot_y(1.0f);
		rot_y = glm::rotate(rot_y, dtf * -.1f, glm::vec3(0.f, 1.f, 0.f));
		sgl::Camera cam(glm::vec3(position * rot_y), { 0.f, 0.f, 0.f });
		device->SetCamera(cam);
	});
	window_->Run();
}

std::shared_ptr<sgl::Mesh> Application::CreateAppleMesh(
	const std::shared_ptr<sgl::Device>& device) const
{
	// Create the physically based rendering program.
	auto simple_program = sgl::CreateProgram("Simple");
	simple_program->UniformMatrix("projection", device->GetProjection());
	simple_program->UniformMatrix("view", device->GetView());
	simple_program->UniformMatrix("model", device->GetModel());

	// Mesh creation.
	auto apple_mesh = std::make_shared<sgl::Mesh>(
		"../Asset/Model/Apple.obj", 
		simple_program);

	// Create the texture and bind it to the mesh.
	auto texture_manager = device->GetTextureManager();
	texture_manager.AddTexture(
		"Color",
		std::make_shared<sgl::Texture>("../Asset/Apple/Color.jpg"));
	apple_mesh->SetTextures({ "Color" });
	device->SetTextureManager(texture_manager);

	return apple_mesh;
}

std::shared_ptr<sgl::Mesh> Application::CreateCubeMapMesh(
	const std::shared_ptr<sgl::Device>& device,
	const std::shared_ptr<sgl::TextureCubeMap>& texture) const
{
	// Create the cube map program.
	auto cubemap_program = sgl::CreateProgram("CubeMap");
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
