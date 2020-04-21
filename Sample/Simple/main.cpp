#include <iostream>
#include <vector>
#include <utility>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "Application.h"
#include "../ShaderGLLib/Window.h"

#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
#else
int main(int ac, char** av)
#endif
{
	try
	{
		Application app(sgl::CreateSDLOpenGL({ 640, 480 }));
		if (!app.Startup())
		{
			return -1;
		}
		app.Run();
	}
	catch (std::exception ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return -2;
	}
	return 0;
}
