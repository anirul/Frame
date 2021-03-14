#include "Window.h"
#include <iostream>
#include <exception>
#include <stdexcept>
#include <chrono>
#include <utility>
#include <sstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <GL/glew.h>
#include "Error.h"
#include "Frame/OpenGL/Device.h"

namespace frame {

	// Private space.
	namespace {

		class SDLOpenGLWindow : public WindowInterface
		{
		public:
			SDLOpenGLWindow(
				const std::pair<std::uint32_t, std::uint32_t> size) :
				size_(size)
			{
				if (SDL_Init(SDL_INIT_VIDEO) != 0)
				{
					ErrorMessageDisplay("Couldn't initialize SDL2.");
					throw std::runtime_error("Couldn't initialize SDL2.");
				}
				sdl_window_ = SDL_CreateWindow(
					"SDL OpenGL",
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

			virtual ~SDLOpenGLWindow()
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
				// FIXME(anirul): May crash in case you resize the window.
				if (draw_interface_)
				{
					// This will call the device startup.
					draw_interface_->Startup(size_);
				}
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
					const double dt = GetFrameDt(time.count());

					// Process events.
					SDL_Event event;
					while (SDL_PollEvent(&event))
					{
						if (!RunEvent(event, dt))
						{
							loop = false;
						}
					}
					if (input_interface_)
						input_interface_->NextFrame();

					// Draw the Scene not used?
					if (draw_interface_)
					{
						draw_interface_->RunDraw(time.count());
					}
					device_->Display(time.count());

					SetWindowTitle(
						"SDL OpenGL - " +
						std::to_string(static_cast<int>(GetFPS(dt))));

					previous_count = time.count();

					// TODO(anirul): Fix me to check which device this is.
					if (device_)
					{
						SDL_GL_SwapWindow(sdl_window_);
					}
				} while (loop);
			}

			void SetDrawInterface(
				const std::shared_ptr<DrawInterface> draw_interface) override
			{
				draw_interface_ = draw_interface;
			}

			void SetInputInterface(
				const std::shared_ptr<InputInterface> input_interface)
				override
			{
				input_interface_ = input_interface;
			}

			void SetUniqueDevice(
				const std::shared_ptr<DeviceInterface> device) override
			{
				device_ = device;
			}

			std::shared_ptr<DeviceInterface> GetUniqueDevice() override
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

			void SetWindowTitle(const std::string& title) const override
			{
				SDL_SetWindowTitle(sdl_window_, title.c_str());
			}

		protected:
			bool RunEvent(const SDL_Event& event, const double dt)
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
				if (input_interface_)
				{
					if (event.type == SDL_KEYDOWN)
					{
						return input_interface_->KeyPressed(
							event.key.keysym.sym,
							dt);
					}
					if (event.type == SDL_KEYUP)
					{
						return input_interface_->KeyReleased(
							event.key.keysym.sym,
							dt);
					}
					if (event.type == SDL_MOUSEMOTION)
					{
						return input_interface_->MouseMoved(
							glm::vec2(event.motion.x, event.motion.y),
							glm::vec2(event.motion.xrel, event.motion.yrel),
							dt);
					}
					if (event.type == SDL_MOUSEBUTTONDOWN)
					{
						return input_interface_->MousePressed(
							SDLButtonToChar(event.button.button),
							dt);
					}
					if (event.type == SDL_MOUSEBUTTONUP)
					{
						return input_interface_->MouseReleased(
							SDLButtonToChar(event.button.button),
							dt);
					}
				}
				return true;
			}

			const char SDLButtonToChar(const Uint8 button) const
			{
				char ret = 0;
				if (button & SDL_BUTTON_LEFT) ret += 1;
				if (button & SDL_BUTTON_RIGHT) ret += 2;
				if (button & SDL_BUTTON_MIDDLE) ret += 4;
				if (button & SDL_BUTTON_X1) ret += 8;
				if (button & SDL_BUTTON_X2) ret += 16;
				return ret;
			}

			void ErrorMessageDisplay(const std::string& message)
			{
#if defined(_WIN32) || defined(_WIN64)
				MessageBox(hwnd_, message.c_str(), "OpenGL Error", 0);
#else
				std::cerr << "OpenGL Error: " << message << std::endl;
#endif
			}

			// Can only be called ONCE per frame!
			const double GetFrameDt(const double t) const
			{
				static double previous_t = 0.0;
				double ret = t - previous_t;
				previous_t = t;
				return ret;
			}

			const double GetFPS(const double dt) const
			{
				return 1.0 / dt;
			}

		private:
			const std::pair<std::uint32_t, std::uint32_t> size_;
			std::shared_ptr<DeviceInterface> device_ = nullptr;
			std::shared_ptr<DrawInterface> draw_interface_ = nullptr;
			std::shared_ptr<InputInterface> input_interface_ = nullptr;
			SDL_Window* sdl_window_ = nullptr;
#if defined(_WIN32) || defined(_WIN64)
			HWND hwnd_ = nullptr;
#endif
		};

		void* InitSDLOpenGLDevice(
			const std::shared_ptr<WindowInterface>& window)
		{
			std::pair<int, int> gl_version;
			// GL context.
			void* gl_context = SDL_GL_CreateContext(
				static_cast<SDL_Window*>(window->GetWindowContext()));
			SDL_GL_SetAttribute(
				SDL_GL_CONTEXT_PROFILE_MASK,
				SDL_GL_CONTEXT_PROFILE_CORE);
			if (!gl_context) return nullptr;
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#if defined(__APPLE__)
			SDL_GL_SetAttribute(
				SDL_GL_CONTEXT_FLAGS,
				SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
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
			// Vsync off.
			SDL_GL_SetSwapInterval(0);

			return gl_context;
		}

	} // End namespace.

	std::shared_ptr<WindowInterface> CreateSDLOpenGL(
		std::pair<std::uint32_t, std::uint32_t> size)
	{
		auto window = std::make_shared<SDLOpenGLWindow>(size);
		auto context = InitSDLOpenGLDevice(window);
		if (!context) return nullptr;
		window->SetUniqueDevice(
			std::make_shared<opengl::Device>(context, size));
		return window;
	}

} // End namespace frame.