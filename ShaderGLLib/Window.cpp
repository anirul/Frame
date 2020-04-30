#include "Window.h"
#include <iostream>
#include <exception>
#include <stdexcept>
#include <chrono>
#include <utility>
#include <sstream>
#include <SDL2/SDL.h>
#include <SDL_syswm.h>
#include <GL/glew.h>
#include "Error.h"

namespace sgl {

	// Private space.
	namespace {

		class SDLWindow : public Window
		{
		public:
			SDLWindow(const std::pair<std::uint32_t, std::uint32_t> size) : 
				size_(size)
			{
				if (SDL_Init(SDL_INIT_VIDEO) != 0)
				{
					ErrorMessageDisplay("Couldn't initialize SDL2.");
					throw std::runtime_error("Couldn't initialize SDL2.");
				}
				sdl_window_ = SDL_CreateWindow(
					"Shader OpenGL",
					SDL_WINDOWPOS_CENTERED,
					SDL_WINDOWPOS_CENTERED,
					size_.first,
					size_.second,
					SDL_WINDOW_OPENGL);
				if (!sdl_window_)
				{
					ErrorMessageDisplay("Couldn't start a window in SDL2.");
					throw std::runtime_error(
						"Couldn't start a window in SDL2.");
				}
#if defined(_WIN32) || defined(_WIN64)
				SDL_SysWMinfo wmInfo;
				SDL_VERSION(&wmInfo.version);
				SDL_GetWindowWMInfo(sdl_window_, &wmInfo);
				hwnd_ = wmInfo.info.win.window;
				Error::SetWindowPtr(hwnd_);
#endif
			}

			virtual ~SDLWindow()
			{
				if (gl_context_)
				{
					SDL_GL_DeleteContext(gl_context_);
				}
				SDL_DestroyWindow(sdl_window_);
				SDL_Quit();
			}

			std::optional<std::pair<int, int>> InitOpenGLDevice()
			{
				std::pair<int, int> gl_version;
				// GL context.
				gl_context_ = SDL_GL_CreateContext(sdl_window_);
				SDL_GL_SetAttribute(
					SDL_GL_CONTEXT_PROFILE_MASK,
					SDL_GL_CONTEXT_PROFILE_CORE);
				if (!gl_context_) return std::nullopt;
#if defined(__APPLE__)
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#else
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
				SDL_GL_GetAttribute(
					SDL_GL_CONTEXT_MAJOR_VERSION,
					&gl_version.first);
				SDL_GL_GetAttribute(
					SDL_GL_CONTEXT_MINOR_VERSION,
					&gl_version.second);
				SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
				// Vsync off.
				SDL_GL_SetSwapInterval(0);

				return gl_version;
			}

			void Startup() override
			{
#if _DEBUG && !defined(__APPLE__)
				// Enable error message.
				glEnable(GL_DEBUG_OUTPUT);
				glDebugMessageCallback(
					SDLWindow::ErrorMessageHandler,
					nullptr);
#endif
			}

			void Run() override
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
						if (!RunEvent(event))
						{
							loop = false;
							continue;
						}
					}

					if (draw_func_)
					{
						draw_func_(time.count());
					}
					GetUniqueDevice()->Draw(time.count());
					previous_count = time.count();
					SDL_GL_SwapWindow(sdl_window_);
				} 
				while (loop);
			}

			void SetDraw(std::function<void(const double)> draw_func) override
			{
				draw_func_ = draw_func;
			}

			std::shared_ptr<Device> GetUniqueDevice() override
			{
				if (!device_)
				{
					device_ = std::make_shared<sgl::Device>(gl_context_, size_);
				}
				return device_;
			}

			std::pair<int, int> GetSize() const override
			{
				return size_;
			}

		protected:
			bool RunEvent(const SDL_Event& event)
			{
				if (event.type == SDL_QUIT)
				{
					return false;
				}
				if (event.type == SDL_KEYDOWN)
				{
					switch (event.key.keysym.sym)
					{
					case SDLK_ESCAPE:
						return false;
					}
				}
				return true;
			}

			void ErrorMessageDisplay(const std::string& message)
			{
#if defined(_WIN32) || defined(_WIN64)
				MessageBox(hwnd_, message.c_str(), "OpenGL Error", 0);
#else
				std::cerr << "OpenGL Error: " << message << std::endl;
#endif
			}

#if !defined(__APPLE__)
			static void GLAPIENTRY ErrorMessageHandler(
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
#endif

		private:
			const std::pair<std::uint32_t, std::uint32_t> size_;
			std::shared_ptr<sgl::Device> device_ = nullptr;
			std::function<void(const double)> draw_func_ = nullptr;
			SDL_Window* sdl_window_ = nullptr;
			void* gl_context_ = nullptr;
#if defined(_WIN32) || defined(_WIN64)
			HWND hwnd_ = nullptr;
#endif
		};

	}

	std::shared_ptr<sgl::Window> CreateSDLOpenGL(std::pair<int, int> size)
	{
		auto window_ptr = std::make_shared<SDLWindow>(size);
		auto maybe_version = window_ptr->InitOpenGLDevice();
		if (!maybe_version) return nullptr;
		return window_ptr;
	}

} // End namespace sgl.