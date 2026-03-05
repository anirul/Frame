// Skinned mesh example (OpenGL-first integration target).
#include <iostream>
#include <exception>
#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "absl/flags/flag.h"
#include "absl/flags/usage.h"

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "frame/common/application.h"
#include "frame/file/file_system.h"
#include "frame/logger.h"

namespace
{

constexpr glm::uvec2 kDefaultSize{1280u, 720u};
constexpr const char* kLevelPath = "asset/json/skinned_mesh.json";
constexpr double kDefaultAutoExitSeconds = 8.0;

int Run(int argc, char** argv)
{
    absl::SetProgramUsageMessage(
        "05_SkinnedMesh [--device={vulkan|opengl}] "
        "[--auto_exit_seconds=<seconds>] (defaults to opengl)");
    std::vector<std::string> args = {};
    args.reserve(static_cast<std::size_t>(argc));
    args.emplace_back(argv[0]);
    bool has_device = false;
    bool has_auto_exit = false;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg(argv[i]);
        args.emplace_back(arg);
        if (arg.rfind("--device", 0) == 0)
        {
            has_device = true;
        }
        if (arg.rfind("--auto_exit_seconds", 0) == 0)
        {
            has_auto_exit = true;
        }
    }
    if (!has_device)
    {
        args.emplace_back("--device=opengl");
    }
    if (!has_auto_exit)
    {
        args.emplace_back(
            std::format("--auto_exit_seconds={}", kDefaultAutoExitSeconds));
    }
    std::vector<char*> app_argv;
    app_argv.reserve(args.size());
    for (auto& arg : args)
    {
        app_argv.push_back(arg.data());
    }
    frame::common::Application app(
        static_cast<int>(app_argv.size()),
        app_argv.data(),
        kDefaultSize);
    const double auto_exit_seconds = absl::GetFlag(FLAGS_auto_exit_seconds);
    app.Startup(frame::file::FindFile(kLevelPath));
    auto& logger = frame::Logger::GetInstance();
    const auto start = std::chrono::steady_clock::now();
    bool auto_exit_logged = false;
    app.Run([start, auto_exit_seconds, &logger, &auto_exit_logged]() {
        if (auto_exit_seconds <= 0.0)
        {
            return true;
        }
        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = now - start;
        const bool keep_running = elapsed.count() < auto_exit_seconds;
        if (!keep_running && !auto_exit_logged)
        {
            logger->info(
                "Auto exit triggered after {:.3f} seconds.",
                auto_exit_seconds);
            logger->flush();
            auto_exit_logged = true;
        }
        return keep_running;
    });
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
    logger->error("Unhandled exception in 05_SkinnedMesh: {}", ex.what());
    logger->flush();
#if defined(_WIN32) || defined(_WIN64)
    MessageBoxA(nullptr, ex.what(), "Error", MB_OK | MB_ICONERROR);
#else
    std::cerr << ex.what() << std::endl;
    std::cerr.flush();
#endif
    return 1;
}
