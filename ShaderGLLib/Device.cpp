#include <exception>
#include <stdexcept>
#include <chrono>
#if defined(_WIN32) || defined(_WIN64)
	#define WINDOWS_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <iostream>
#endif
#include <glm/gtc/matrix_transform.hpp>
#include "Device.h"

namespace sgl {

	Device::Device(void* gl_context) : gl_context_(gl_context) 
	{
		// Initialize GLEW.
		if (GLEW_OK != glewInit())
		{
			throw std::runtime_error("couldn't initialize GLEW");
		}

		// Enable blending to 1 - source alpha.
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Enable Z buffer.
		glEnable(GL_DEPTH_TEST);
	}

	void Device::Startup(std::pair<int, int> size)
	{
		// Create a program.
		program_ = std::make_shared<Program>();

		// Vertex Shader program.
		sgl::Shader vertex_shader(ShaderType::VERTEX_SHADER);
		if (!vertex_shader.LoadFromFile("../Asset/PBRVertex.glsl"))
		{
			throw std::runtime_error(
				"Fragment shader Error: " + vertex_shader.GetErrorMessage());
		}

		// Fragment Shader program.
		sgl::Shader fragment_shader(ShaderType::FRAGMENT_SHADER);
		if (!fragment_shader.LoadFromFile("../Asset/PBRFragment.glsl"))
		{
			throw std::runtime_error(
				"Fragment shader Error: " + fragment_shader.GetErrorMessage());
		}

		// Create the program.
		program_->AddShader(vertex_shader);
		program_->AddShader(fragment_shader);
		program_->LinkShader();
		program_->Use();

		// Set the perspective matrix.
		const float aspect =
			static_cast<float>(size.first) / static_cast<float>(size.second);
		glm::mat4 perspective = glm::perspective(
			glm::radians(65.0f),
			aspect,
			0.1f,
			100.0f);
		program_->UniformMatrix("projection", perspective);

		// Set the camera.
		glm::mat4 view = camera_.LookAt();
		program_->UniformMatrix("view", view);

		// Set the model matrix (identity for now).
		glm::mat4 model(1.0f);
		program_->UniformMatrix("model", model);

		// Set the camera and light uniform!
		SetCamera(camera_);
		const float light_value = 300.f;
		const glm::vec3 light_vec(light_value, light_value, light_value);
		AddLight({ -10.f,  10.f,  10.f }, light_vec);
		AddLight({ 10.f,  10.f,  10.f }, light_vec);
		AddLight({ -10.f,  -10.f,  10.f }, light_vec);
		AddLight({ 10.f,  -10.f,  10.f }, light_vec);
	}

	void Device::Draw(const double dt)
	{
		// Clear the screen.
		glClearColor(.2f, 0.f, .2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (const std::shared_ptr<sgl::Scene>& scene : scene_tree_)
		{
			const std::shared_ptr<sgl::Mesh>& mesh = scene->GetLocalMesh();
			if (!mesh)
			{
				continue;
			}

			// Draw the mesh.
			mesh->Draw(*program_, texture_manager_, scene->GetLocalModel(dt));
		}
	}

	bool Device::AddLight(const glm::vec3& position, const glm::vec3& color)
	{
		if (light_count >= max_light_count) return false;
		program_->UniformVector3(
			"light_position[" + std::to_string(light_count) + "]", 
			position);
		program_->UniformVector3(
			"light_color[" + std::to_string(light_count) + "]", 
			color);
		light_count++;
		return true;
	}

	void Device::SetCamera(const sgl::Camera& camera)
	{
		camera_ = camera;
		program_->UniformVector3("camera_position", camera_.Position());
	}

} // End namespace sgl.
