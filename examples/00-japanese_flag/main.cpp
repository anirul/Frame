#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "frame/common/application.h"
#include "frame/file/file_system.h"
#include "frame/file/image_stb.h"
#include "frame/window_factory.h"

namespace
{
constexpr glm::uvec2 kDefaultSize{1280u, 720u};
constexpr const char* kLevelPath = "asset/json/japanese_flag.json";

frame::WindowReturnEnum RunApplication(frame::RenderingAPIEnum api)
{
    frame::common::Application app(frame::CreateNewWindow(
        frame::DrawingTargetEnum::WINDOW,
        api,
        kDefaultSize));
    app.Startup(frame::file::FindFile(kLevelPath));
    return app.Run();
}

std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

frame::RenderingAPIEnum ParseRequestedDevice(const std::vector<std::string>& args)
{
    for (std::size_t i = 0; i < args.size(); ++i)
    {
        const std::string& arg = args[i];
        if (arg == "-device" || arg == "--device")
        {
            if (i + 1 >= args.size())
            {
                break;
            }
            const std::string next = ToLowerCopy(args[i + 1]);
            if (next == "opengl")
            {
                return frame::RenderingAPIEnum::OPENGL;
            }
            if (next == "vulkan")
            {
                return frame::RenderingAPIEnum::VULKAN;
            }
        }
    }
    return frame::RenderingAPIEnum::VULKAN;
}

frame::WindowReturnEnum RunWithPreference(const std::vector<std::string>& args)
{
    const frame::RenderingAPIEnum requested = ParseRequestedDevice(args);
    if (requested == frame::RenderingAPIEnum::VULKAN)
    {
        try
        {
            return RunApplication(frame::RenderingAPIEnum::VULKAN);
        }
        catch (const std::exception& ex)
        {
#if defined(_WIN32) || defined(_WIN64)
            OutputDebugStringA("Vulkan startup failed, falling back to OpenGL.\n");
            OutputDebugStringA(ex.what());
            OutputDebugStringA("\n");
#else
            std::cerr << "Vulkan startup failed (" << ex.what()
                      << "), falling back to OpenGL." << std::endl;
#endif
        }
    }
    return RunApplication(frame::RenderingAPIEnum::OPENGL);
}

#if defined(_WIN32) || defined(_WIN64)
std::vector<std::string> CollectCommandLineArgs()
{
    int argc = 0;
    LPWSTR* argv_w = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> args;
    if (!argv_w)
    {
        return args;
    }
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i)
    {
        const int required = WideCharToMultiByte(
            CP_UTF8,
            0,
            argv_w[i],
            -1,
            nullptr,
            0,
            nullptr,
            nullptr);
        if (required <= 0)
        {
            continue;
        }
        std::string utf8(static_cast<std::size_t>(required - 1), '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            argv_w[i],
            -1,
            utf8.data(),
            required,
            nullptr,
            nullptr);
        args.push_back(std::move(utf8));
    }
    LocalFree(argv_w);
    return args;
}
#else
std::vector<std::string> CollectCommandLineArgs(int argc, char** argv)
{
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i)
    {
        args.emplace_back(argv[i]);
    }
    return args;
}
#endif

} // namespace

#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(
    _In_ HINSTANCE /*hInstance*/,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPSTR /*lpCmdLine*/,
    _In_ int /*nShowCmd*/)
try
{
    const auto args = CollectCommandLineArgs();
    RunWithPreference(args);
    return 0;
}
#else
int main(int argc, char** argv)
try
{
    const auto args = CollectCommandLineArgs(argc, argv);
    RunWithPreference(args);
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
