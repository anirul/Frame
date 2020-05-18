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
				// TODO(anirul): Fix me to check which device this is.
				if (device_)
				{
					SDL_GL_DeleteContext(device_->GetDeviceContext());
				}
				SDL_DestroyWindow(sdl_window_);
				SDL_Quit();
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

					auto texture = device_->DrawTexture(time.count());
					if (draw_func_)
					{
						draw_func_(time.count(), texture);
					}
					device_->Display(texture);

					previous_count = time.count();
					// TODO(anirul): Fix me to check which device this is.
					if (device_)
					{
						SDL_GL_SwapWindow(sdl_window_);
					}
				} 
				while (loop);
			}

			void SetDraw(
				std::function<void(
					const double, 
					std::shared_ptr<Texture>&)> draw_func) override
			{
				draw_func_ = draw_func;
			}

			void SetUniqueDevice(const std::shared_ptr<Device>& device) override
			{
				device_ = device;
			}

			std::shared_ptr<Device> GetUniqueDevice() override
			{
				return device_;
			}

			std::pair<std::uint32_t, std::uint32_t> GetSize() const override
			{
				return size_;
			}

			void* GetWindowContext() const override
			{
				return sdl_window_;
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

		private:
			const std::pair<std::uint32_t, std::uint32_t> size_;
			std::shared_ptr<sgl::Device> device_ = nullptr;
			std::function<void(
				const double, 
				std::shared_ptr<Texture>&)> draw_func_ = nullptr;
			SDL_Window* sdl_window_ = nullptr;
#if defined(_WIN32) || defined(_WIN64)
			HWND hwnd_ = nullptr;
#endif
		};

		void* InitOpenGLDevice(
			const std::shared_ptr<Window>& window)
		{
			std::pair<int, int> gl_version;
			// GL context.
			void* gl_context = SDL_GL_CreateContext(
				static_cast<SDL_Window*>(window->GetWindowContext()));
			SDL_GL_SetAttribute(
				SDL_GL_CONTEXT_PROFILE_MASK,
				SDL_GL_CONTEXT_PROFILE_CORE);
			if (!gl_context) return nullptr;
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

			return gl_context;
		}

	} // End namespace.

	std::shared_ptr<sgl::Window> CreateSDLOpenGL(
		std::pair<std::uint32_t, std::uint32_t> size)
	{
		auto window = std::make_shared<SDLWindow>(size);
		auto context = InitOpenGLDevice(window);
		if (!context) return nullptr;
		window->SetUniqueDevice(std::make_shared<Device>(context, size));
		return window;
	}

} // End namespace sgl.