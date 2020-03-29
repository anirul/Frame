#include <exception>
#include <stdexcept>
#include <chrono>
#if defined(_WIN32) || defined(_WIN64)
	#define WINDOWS_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <iostream>
#endif
#include "Device.h"

namespace sgl {

	Device::Device(SDL_Window* sdl_window) 
	{
		// GL context.
		sdl_gl_context_ = SDL_GL_CreateContext(sdl_window);
		SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_PROFILE_MASK,
			SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
#if defined(__APPLE__)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major_version_);
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor_version_);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		// Vsync off.
		SDL_GL_SetSwapInterval(0);

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
		sgl::matrix perspective = sgl::Perspective(
			65.0f * static_cast<float>(M_PI) / 180.0f,
			aspect,
			0.1f,
			100.0f);
		program_->UniformMatrix("projection", perspective);

		// Set the camera.
		sgl::matrix view = camera_.LookAt();
		program_->UniformMatrix("view", view);

		// Set the model matrix (identity for now).
		sgl::matrix model = {};
		program_->UniformMatrix("model", model);

		// Set the camera and light uniform!
		SetCamera(camera_);
		SetLight({ -10.f, 10.f, 10.f }, { 300.f, 300.f, 300.f });
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

	void Device::SetLight(const sgl::vector3 position, const sgl::vector3 color)
	{
		program_->UniformVector3("light_position", position);
		program_->UniformVector3("light_color", color);
	}

	void Device::SetCamera(const sgl::Camera& camera)
	{
		camera_ = camera;
		program_->UniformVector3("camera_position", camera_.Position());
	}

} // End namespace sgl.
