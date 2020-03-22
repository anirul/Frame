#pragma once

#include <gtest/gtest.h>
#include <SDL2/sdl.h>
#include <gl/glew.h>

namespace test {

	class OpenGLTest : public testing::Test
	{
	public:
		OpenGLTest()
		{
			if (SDL_Init(SDL_INIT_VIDEO) != 0)
			{
				throw std::runtime_error("Couldn't initialize SDL2.");
			}
			sdl_window_ = SDL_CreateWindow(
				"Shader OpenGL",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				640,
				480,
				SDL_WINDOW_OPENGL);
			if (!sdl_window_)
			{
				throw std::runtime_error("Couldn't start a window in SDL2.");
			}
		}
		~OpenGLTest()
		{
			SDL_DestroyWindow(sdl_window_);
		}
		void GLContextAndGlewInit()
		{
			// GL context.
			auto sdl_gl_context = SDL_GL_CreateContext(sdl_window_);
			SDL_GL_SetAttribute(
				SDL_GL_CONTEXT_PROFILE_MASK,
				SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
#if defined(__APPLE__)
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

			// Initialize GLEW.
			if (GLEW_OK != glewInit())
			{
				throw std::runtime_error("couldn't initialize GLEW");
			}
			PostGlewInit();
		}
		void PostGlewInit()
		{
#if _DEBUG && !defined(__APPLE__)
			// Enable error message.
			glEnable(GL_DEBUG_OUTPUT);
			glDebugMessageCallback(OpenGLTest::ErrorMessageHandler, nullptr);
#endif
		}
#if _DEBUG && !defined(__APPLE__)
		static void GLAPIENTRY ErrorMessageHandler(
			GLenum source,
			GLenum type,
			GLuint id,
			GLenum severity,
			GLsizei length,
			const GLchar* message,
			const void* userParam)
		{
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
			// Remove notifications.
			if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
			{
				std::cerr << "OpenGL Error: " << oss.str() << std::endl;
				FAIL();
			}
		}
#endif

	protected:
		SDL_Window* sdl_window_ = nullptr;
	};

} // End namespace test.
