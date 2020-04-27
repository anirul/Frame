#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>

Application::Application(const std::shared_ptr<sgl::Window>& window) :
	window_(window) {}

bool Application::Startup()
{
	auto device = window_->CreateDevice();
	device->Startup();
	auto billboard_mesh = CreateBillboardMesh();
	auto billboard_program = billboard_mesh->GetProgram();

	// Comment out if you want to see the errors.
	// window_->Startup();

	// Pack it into a Scene object.
	sgl::SceneTree scene_tree{};
	auto root = std::make_shared<sgl::SceneMatrix>(
		[billboard_program](const double dt) -> glm::mat4
	{
		billboard_program->UniformFloat("Time", static_cast<float>(dt));
		return glm::mat4(1.0);
	});
	scene_tree.AddNode(root);
	scene_tree.AddNode(std::make_shared<sgl::SceneMesh>(billboard_mesh), root);
	device->SetSceneTree(scene_tree);

	return true;
}

void Application::Run()
{
	window_->Run();
}

std::shared_ptr<sgl::Mesh> Application::CreateBillboardMesh() const
{
	// Create the ray marching program.
	auto ray_marshing_program = sgl::CreateProgram("JapaneseFlag");

	std::vector<float> points =
	{
		-1.f, 1.f, 0.f,
		1.f, 1.f, 0.f,
		-1.f, -1.f, 0.f,
		1.f, -1.f, 0.f,
	};
	std::vector<float> normals =
	{
		0.f, 0.f, 1.f,
		0.f, 0.f, 1.f,
		0.f, 0.f, 1.f,
		0.f, 0.f, 1.f,
	};
	std::vector<float> texcoords =
	{
		0, 0,
		1, 0,
		0, 1,
		1, 1,
	};
	std::vector<std::int32_t> indices =
	{
		0, 1, 2,
		1, 3, 2,
	};
	auto billboard_mesh = std::make_shared<sgl::Mesh>(
		points,
		normals,
		texcoords,
		indices,
		ray_marshing_program);
	return billboard_mesh;
}
