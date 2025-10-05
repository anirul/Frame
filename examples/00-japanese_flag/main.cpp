#include <iostream>
#include <string>

#include <glm/glm.hpp>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "frame/common/application.h"
#include "frame/file/file_system.h"

namespace
{
constexpr glm::uvec2 kDefaultSize{1280u, 720u};
constexpr const char* kLevelPath = "asset/json/japanese_flag.json";
}

#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(
    _In_ HINSTANCE /*hInstance*/,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPSTR /*lpCmdLine*/,
    _In_ int /*nShowCmd*/)
try
{
    frame::common::Application app(__argc, __argv, kDefaultSize);
    app.Startup(frame::file::FindFile(kLevelPath));
    app.Run();
    return 0;
}
#else
int main(int argc, char** argv)
try
{
    frame::common::Application app(argc, argv, kDefaultSize);
    app.Startup(frame::file::FindFile(kLevelPath));
    app.Run();
    return 0;
}
#endif
catch (const std::exception& ex)
{
#if defined(_WIN32) || defined(_WIN64)
    MessageBox(nullptr, ex.what(), "Exception", MB_ICONEXCLAMATION);
#else
    std::cerr << "Error: " << ex.what() << std::endl;
#endif
    return -2;
}
