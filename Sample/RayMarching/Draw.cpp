#include "Draw.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	auto billboard_mesh = CreateBillboardMesh();

	// Pack it into a Scene object.
	sgl::SceneTree scene_tree{};
	auto root = std::make_shared<sgl::SceneMatrix>(glm::mat4(1.0));
	scene_tree.AddNode(root);
	scene_tree.AddNode(std::make_shared<sgl::SceneMesh>(billboard_mesh), root);
	device_->SetSceneTree(scene_tree);

	out_texture_ = std::make_shared<sgl::Texture>(size);
}

void Draw::RunDraw(const double dt)
{
	program_->Use();
	program_->Uniform("Time", static_cast<float>(dt));
	device_->DrawMultiTextures(program_, { out_texture_ }, dt);
}

const std::shared_ptr<sgl::Texture> Draw::GetDrawTexture() const
{
	return out_texture_;
}

std::shared_ptr<sgl::Mesh> Draw::CreateBillboardMesh()
{
	// Create the ray marching program.
	program_ = sgl::CreateProgram("RayMarching");
	auto billboard_mesh = sgl::CreateQuadMesh();
	return billboard_mesh;
}
