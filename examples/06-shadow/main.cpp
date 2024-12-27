// From: https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <frame/common/application.h>
#include <frame/file/file_system.h>
#include <frame/file/image_stb.h>
#include <frame/window_factory.h>

#include <exception>
#include <glm/glm.hpp>

// From: https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
try
{
    int ac = __argc;
    char** av = __argv;
#else
int main(int ac, char** av)
try
{
#endif
    frame::common::Application app(frame::CreateNewWindow(
        frame::DrawingTargetEnum::WINDOW,
        frame::RenderingAPIEnum::OPENGL,
        {1280, 720}));
    app.Startup(frame::file::FindFile("asset/json/shadow.json"));
    app.Run();
    return 0;
}
catch (const std::exception& e)
{
#if defined(_WIN32) || defined(_WIN64)
    MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
#else
    std::cerr << e.what() << std::endl;
#endif
    return 1;
}
