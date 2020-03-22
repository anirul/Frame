#include "WindowSoftwareGL.h"

#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <SDL.h>

bool WindowSoftwareGL::Startup(const std::pair<int, int>& gl_version)
{
	return (gl_version.first >= 4) && (gl_version.second >= 1);
}

bool WindowSoftwareGL::RunCompute(const double delta_time)
{
	return true;
}

bool WindowSoftwareGL::RunEvent(const SDL_Event& event)
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

const std::pair<int, int> WindowSoftwareGL::GetWindowSize() const
{
	return std::make_pair(width_, height_);
}
