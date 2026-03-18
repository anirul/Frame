// Skinned mesh example using the shared ray tracing pipeline.
#include <iostream>

#include <glm/glm.hpp>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "absl/flags/usage.h"

#include "frame/common/application.h"
#include "frame/file/file_system.h"
#include "frame/logger.h"

namespace
{
constexpr glm::uvec2 kDefaultSize{1280u, 720u};
constexpr const char* kLevelPath = "asset/json/skinned_mesh.json";

int Run(int argc, char** argv)
{
    absl::SetProgramUsageMessage(
        "03_SkinnedMesh --device={vulkan|opengl} "
        "[--auto_exit_seconds=<seconds>] (defaults to vulkan)");
    frame::common::Application app(argc, argv, kDefaultSize);
    app.Startup(frame::file::FindFile(kLevelPath));
    app.Run();
    return 0;
}
} // namespace

#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(
    _In_ HINSTANCE /*hInstance*/,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPSTR /*lpCmdLine*/,
    _In_ int /*nShowCmd*/)
try
{
    return Run(__argc, __argv);
}
#else
int main(int argc, char** argv)
try
{
    return Run(argc, argv);
}
#endif
catch (const std::exception& ex)
{
    auto& logger = frame::Logger::GetInstance();
    logger->error("Unhandled exception in 03_SkinnedMesh: {}", ex.what());
    logger->flush();
#if defined(_WIN32) || defined(_WIN64)
    MessageBoxA(nullptr, ex.what(), "Error", MB_OK | MB_ICONERROR);
#else
    std::cerr << ex.what() << std::endl;
    std::cerr.flush();
#endif
    return 1;
}
