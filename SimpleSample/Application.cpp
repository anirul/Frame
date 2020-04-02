#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>

Application::Application(const std::shared_ptr<sgl::Window>& window) :
	window_(window) {}

bool Application::Startup()
{
	auto device = window_->CreateDevice();

	sgl::Shader vertex(sgl::ShaderType::VERTEX_SHADER);
	sgl::Shader fragment(sgl::ShaderType::FRAGMENT_SHADER);
	if (!vertex.LoadFromFile("../Asset/PBR.Vertex.glsl"))
	{
		throw 
			std::runtime_error(
				"Error loading vertex shader: " + 
				vertex.GetErrorMessage());
	}
	if (!fragment.LoadFromFile("../Asset/PBR.Fragment.glsl"))
	{
		throw
			std::runtime_error(
				"Error loading fragment shader: " +
				fragment.GetErrorMessage());
	}
	device->Startup(window_->GetSize(), vertex, fragment);
	
	// Mesh creation.
	auto gl_mesh = std::make_shared<sgl::Mesh>("../Asset/Apple.obj");

	// Create the texture and bind it to the mesh.
	sgl::TextureManager texture_manager{};
	texture_manager.AddTexture(
		"Color",
		std::make_shared<sgl::Texture>(
			"../Asset/Apple/Color.jpg"));
	texture_manager.AddTexture(
		"Normal",
		std::make_shared<sgl::Texture>(
			"../Asset/Apple/Normal.jpg"));
	texture_manager.AddTexture(
		"Metallic",
		std::make_shared<sgl::Texture>(
			"../Asset/Apple/Metalness.jpg"));
	texture_manager.AddTexture(
		"Roughness",
		std::make_shared<sgl::Texture>(
			"../Asset/Apple/Roughness.jpg"));
	texture_manager.AddTexture(
		"AmbientOcclusion",
		std::make_shared<sgl::Texture>(
			"../Asset/Apple/AmbientOcclusion.jpg"));
	gl_mesh->SetTextures(
		{ "Color", "Normal", "Metallic", "Roughness", "AmbientOcclusion" });
	device->SetTextureManager(texture_manager);

	// Pack it into a Scene object.
	sgl::SceneTree scene_tree{};
	{
		auto scene_matrix = std::make_shared<sgl::SceneMatrix>(
			[](const double dt) -> glm::mat4
		{
			glm::mat4 r_x(1.0f);
			glm::mat4 r_y(1.0f);
			glm::mat4 r_z(1.0f);
			const auto dtf = static_cast<float>(dt);
			r_x = glm::rotate(r_x, dtf * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));
			r_y = glm::rotate(r_y, dtf * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
			r_z = glm::rotate(r_z, dtf       , glm::vec3(0.0f, 0.0f, 1.0f));
			return r_x * r_y * r_z;
		});
		scene_tree.AddNode(scene_matrix);
		scene_tree.AddNode(
			std::make_shared<sgl::SceneMesh>(gl_mesh),
			scene_matrix);
	}
	device->SetSceneTree(scene_tree);
	window_->Startup();
	return true;
}

void Application::Run()
{
	window_->Run();
}
