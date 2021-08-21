#include "Error.h"

#include <stdexcept>
#include <GL/glew.h>
#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace frame {

	void* Error::window_ptr_ = nullptr;

	std::string Error::GetLastError() const
	{
		// FIXME(anirul): This should be in FrameOpenGL and not here.
		switch (glGetError())
		{
		case GL_INVALID_ENUM:
			return "Invalid enum";
		case GL_INVALID_VALUE:
			return "Invalid value";
		case GL_INVALID_OPERATION:
			return "Invalid operation";
		case GL_INVALID_FRAMEBUFFER_OPERATION:
		{
			switch (glCheckFramebufferStatus(GL_FRAMEBUFFER))
			{
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				return "Frame buffer incomplete attachment";
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
				return "Frame buffer incomplete dimensions";
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				return "Frame buffer incomplete missing attachment";
			case GL_FRAMEBUFFER_UNSUPPORTED:
				return "Frame buffer unsupported";
			case GL_FRAMEBUFFER_COMPLETE:
				return "";
			}
		}
		case GL_OUT_OF_MEMORY:
			return "Out of memory";
		case GL_NO_ERROR:
			return "";
		}
		return "";
	}

	void Error::Display(
		const std::string& file /*= ""*/,
		const int line /*= -1*/) const
	{
#ifdef _DEBUG
		std::string error = GetLastError();
		if (error.empty()) return;
		CreateError(error, file, line);
#endif
	}

	void Error::CreateError(
		const std::string& error, 
		const std::string& file, 
		const int line /*= -1*/) const
	{
		std::string temporary_error = error;
		if (line != -1) 
		{
			temporary_error = std::to_string(line) + "\n" + temporary_error;
		}
		if (!file.empty()) temporary_error = file + "\n" + temporary_error;
		throw std::runtime_error(temporary_error);
	}

} // End namespace frame.
