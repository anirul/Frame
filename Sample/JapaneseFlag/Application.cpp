#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>

Application::Application(const std::shared_ptr<sgl::Window>& window) :
	window_(window) {}

bool Application::Startup()
{
	auto device = window_->GetUniqueDevice();
	device->Startup();
	auto billboard_mesh = CreateBillboardMesh();
	auto billboard_program = billboard_mesh->GetProgram();

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
	auto billboard_mesh = sgl::CreateQuadMesh(ray_marshing_program);
	return billboard_mesh;
}
