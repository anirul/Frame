#include "WindowSDL2GL.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <array>
#include <algorithm>
#include <numeric>
#include <functional>
#include <set>
#include <sdl2/SDL.h>
#include <GL/glew.h>
#if defined(_WIN32) || defined(_WIN64)
	#include <SDL2/SDL_syswm.h>
#endif
#include "../ShaderGLLib/Shader.h"
#include "../ShaderGLLib/Device.h"
#include "../ShaderGLLib/Texture.h"

namespace SoftwareGL {

#if !defined(__APPLE__)
	void GLAPIENTRY WindowSDL2GL::ErrorMessageHandler(
		GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam) 
	{
		// Remove notifications.
 		if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) 
 			return;
		// Remove medium
		if (severity == GL_DEBUG_SEVERITY_MEDIUM)
			return;
		std::ostringstream oss;
		oss << "message\t: " << message << std::endl;
		oss << "type\t: ";
		switch (type) 
		{
		case GL_DEBUG_TYPE_ERROR:
			oss << "ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			oss << "DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			oss << "PORABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			oss << "PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER:
			oss << "OTHER";
			break;
		}
		oss << std::endl;
		oss << "id\t: " << id << std::endl;
		oss << "severity\t: ";
		switch (severity) 
		{
		case GL_DEBUG_SEVERITY_LOW:
			oss << "LOW";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			oss << "MEDIUM";
			break;
		case GL_DEBUG_SEVERITY_HIGH:
			oss << "HIGH";
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			oss << "NOTIFICATION";
			break;
		}
		oss << std::endl;
#if defined(_WIN32) || defined(_WIN64)
		MessageBox(nullptr, oss.str().c_str(), "OpenGL Error", 0);
#else
		std::cerr << "OpenGL Error: " << oss.str() << std::endl;
#endif
	}

	void WindowSDL2GL::ErrorMessageDisplay(const std::string& error)
	{
#if defined(_WIN32) || defined(_WIN64)
		MessageBox(hwnd_, error.c_str(), "OpenGL Error", 0);
#else
		std::cerr << "OpenGL Error: " << error << std::endl;
#endif
	}
#endif

	WindowSDL2GL::WindowSDL2GL(
		std::shared_ptr<WindowInterface> window_interface) :
		window_interface_(window_interface) 
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			ErrorMessageDisplay("Couldn't initialize SDL2.");
			throw std::runtime_error("Couldn't initialize SDL2.");
		}
		const auto p_size = window_interface_->GetWindowSize();
		sdl_window_ = SDL_CreateWindow(
			"Shader OpenGL",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			static_cast<int>(p_size.first),
			static_cast<int>(p_size.second),
			SDL_WINDOW_OPENGL);
		if (!sdl_window_)
		{
			ErrorMessageDisplay("Couldn't start a window in SDL2.");
			throw std::runtime_error("Couldn't start a window in SDL2.");
		}
#if defined(_WIN32) || defined(_WIN64)
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(sdl_window_, &wmInfo);
		hwnd_ = wmInfo.info.win.window;
#endif
		// Create a new device.
		device_ = std::make_shared<sgl::Device>(sdl_window_);
	}

	WindowSDL2GL::~WindowSDL2GL()
	{
		SDL_Quit();
	}

	void WindowSDL2GL::PostRunCompute(const double dt)
	{
		// Draw the new view.
		device_->Draw(dt);
	}

	bool WindowSDL2GL::Startup()
	{
#if _DEBUG && !defined(__APPLE__)
		// Enable error message.
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(WindowSDL2GL::ErrorMessageHandler, nullptr);
#endif
		// Start the user part of the window.
		if (!window_interface_->Startup(device_->GetGLVersion()))
		{
			auto p = device_->GetGLVersion();
			std::string error = "Error version is too low (" +
				std::to_string(p.first) + ", " +
				std::to_string(p.second) + ")";
			ErrorMessageDisplay(error);
			return false;
		}

		// Device Startup call.
		device_->Startup(window_interface_->GetWindowSize());

		// Mesh creation.
		auto gl_mesh = 
			std::make_shared<sgl::Mesh>("../Asset/TorusUVNormal.obj");

		// Create the texture and bind it to the mesh.
		sgl::TextureManager texture_manager{};
		texture_manager.AddTexture(
			"Color",
			std::make_shared<sgl::Texture>("../Asset/Planks/Color.jpg"));
		texture_manager.AddTexture(
			"Normal",
			std::make_shared<sgl::Texture>("../Asset/Planks/Normal.jpg"));
		texture_manager.AddTexture(
			"Metallic",
			std::make_shared<sgl::Texture>("../Asset/Planks/Metalness.jpg"));
		texture_manager.AddTexture(
			"Roughness",
			std::make_shared<sgl::Texture>("../Asset/Planks/Roughness.jpg"));
		texture_manager.AddTexture(
			"AmbientOcclusion",
			std::make_shared<sgl::Texture>(
				"../Asset/Planks/AmbientOcclusion.jpg"));
		gl_mesh->SetTextures(
			{ "Color", "Normal", "Metallic", "Roughness", "AmbientOcclusion" });
		device_->SetTextureManager(texture_manager);

		// Pack it into a Scene object.
		sgl::SceneTree scene_tree{};
		{
			auto scene_matrix = std::make_shared<sgl::SceneMatrix>(
				[](const double dt) 
			{
				sgl::matrix r_x;
				sgl::matrix r_y;
				sgl::matrix r_z;
				const auto dtf = static_cast<float>(dt);
				r_x.RotateXMatrix(dtf * 0.7f);
				r_y.RotateYMatrix(dtf * 0.5f);
				r_z.RotateZMatrix(dtf);
				return r_x * r_y * r_z;
			});
			scene_tree.AddNode(scene_matrix);
			scene_tree.AddNode(
				std::make_shared<sgl::SceneMesh>(gl_mesh), 
				scene_matrix);
		}
		device_->SetSceneTree(scene_tree);

		return true;
	}

	void WindowSDL2GL::Run()
	{
		// While Run return true continue.
		bool loop = true;
		double previous_count = 0.0f;
		// Timing counter.
		static auto start = std::chrono::system_clock::now();
		do 
		{
			// Compute the time difference from previous frame.
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> time = end - start;
			// Process events
			SDL_Event event;
			if (SDL_PollEvent(&event))
			{
				if (!window_interface_->RunEvent(event))
				{
					loop = false;
					continue;
				}
			}

			if (!window_interface_->RunCompute(time.count() - previous_count))
			{
				loop = false;
				continue;
			}
			PostRunCompute(time.count());
			previous_count = time.count();
			SDL_GL_SwapWindow(sdl_window_);
		} 
		while (loop);
		window_interface_->Cleanup();
		// Cleanup.
		SDL_GL_DeleteContext(sdl_gl_context_);
		SDL_DestroyWindow(sdl_window_);
		SDL_Quit();
	}

}	// End namespace SoftwareGL.
