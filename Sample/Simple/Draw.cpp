#include "Draw.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size) 
{
	auto texture = std::make_shared<sgl::TextureCubeMap>(
		std::array<std::string, 6>{
		"../Asset/CubeMap/NegativeX.png", "../Asset/CubeMap/PositiveX.png",
		"../Asset/CubeMap/NegativeY.png", "../Asset/CubeMap/PositiveY.png",
		"../Asset/CubeMap/NegativeZ.png", "../Asset/CubeMap/PositiveZ.png",
	});
	apple_mesh_ = CreateAppleMesh();
	cube_map_mesh_ = CreateCubeMapMesh(texture);

	// Pack it into a Scene object.
	sgl::SceneTree scene_tree{};
	{
		auto scene_root = std::make_shared<sgl::SceneMatrix>(glm::mat4(
			1.f, 0.f, 0.f, 0.f,
			// TODO(anirul): Why is this inverted?
			0.f, -1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f));
		scene_tree.AddNode(scene_root);
		scene_tree.AddNode(
			std::make_shared<sgl::SceneMesh>(cube_map_mesh_),
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
			std::make_shared<sgl::SceneMesh>(apple_mesh_),
			scene_matrix);
	}
	device_->SetSceneTree(scene_tree);

	out_textures_.emplace_back(std::make_shared<sgl::Texture>(size));
	out_textures_.emplace_back(std::make_shared<sgl::Texture>(size));
}

const std::shared_ptr<sgl::Texture> Draw::GetDrawTexture() const
{
	return out_textures_[0];
}

void Draw::RunDraw(const double dt)
{
	float dtf = static_cast<float>(dt);
	glm::vec4 position = { 0.f, 0.f, 2.f, 0.f };
	glm::vec4 front = { 0.f, 0.f, -1.f, 0.f };
	glm::mat4 rot_y(1.0f);
	rot_y = glm::rotate(rot_y, dtf * -.1f, glm::vec3(0.f, 1.f, 0.f));
	sgl::Camera cam(glm::vec3(position * rot_y), glm::vec3(front * rot_y));
	device_->SetCamera(cam);
	// Set the uniform for the cubemap program.
	cubemap_program_->Use();
	cubemap_program_->Uniform("projection", device_->GetProjection());
	// Set the uniform for the simple program.
	simple_program_->Use();
	simple_program_->Uniform("projection", device_->GetProjection());
	simple_program_->Uniform("view", device_->GetView());
	simple_program_->Uniform("model", device_->GetModel());
	// Draw the apple.
	// CHECKME(anirul): Shouldn't we first render the skybox?
	device_->DrawMultiTextures(simple_program_, out_textures_, dt);
}

std::shared_ptr<sgl::Mesh> Draw::CreateAppleMesh()
{
	// Create the physically based rendering program.
	simple_program_ = sgl::CreateProgram("Simple");

	// Mesh creation.
	auto apple_mesh = sgl::CreateMeshFromObjFile("../Asset/Model/Torus.obj");

	// Create the texture and bind it to the mesh.
	auto material = std::make_shared<sgl::Material>();
	material->AddTexture(
		"Color",
		std::make_shared<sgl::Texture>("../Asset/Apple/Color.jpg"));
	apple_mesh->SetMaterial(material);
	
	return apple_mesh;
}

std::shared_ptr<sgl::Mesh> Draw::CreateCubeMapMesh(
	const std::shared_ptr<sgl::TextureCubeMap>& texture)
{
	// Create the cube map program.
	cubemap_program_ = sgl::CreateProgram("CubeMap");

	// Create the mesh for the cube.
	auto cube_mesh = sgl::CreateCubeMesh();

	// Get the texture manager.
	auto material = std::make_shared<sgl::Material>();
	material->AddTexture("Skybox", texture);
	cube_mesh->SetMaterial(material);

	// Enable the cleaning of the depth.
	cube_mesh->ClearDepthBuffer(true);

	return cube_mesh;
}