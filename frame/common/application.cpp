#include "frame/common/application.h"

#include <chrono>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/ascii.h"

#include "frame/common/draw.h"
#include "frame/logger.h"
#include "frame/window_factory.h"
#include "frame/vulkan/window_factory.h"

ABSL_FLAG(std::string, device, "vulkan", "Rendering backend (vulkan|opengl).");
ABSL_FLAG(bool, vk_validation, false, "Enable Vulkan validation layers.");
ABSL_FLAG(
    double,
    auto_exit_seconds,
    0.0,
    "Auto-exit executable after N seconds (0 disables).");
ABSL_FLAG(
    bool,
    screenshot_on_exit,
    false,
    "Save ScreenShot.png immediately before auto-exit triggers.");

namespace
{

bool StartsWith(std::string_view value, std::string_view prefix)
{
    return value.size() >= prefix.size() &&
           value.substr(0, prefix.size()) == prefix;
}

std::string NormalizeKnownFlag(std::string_view arg)
{
    constexpr std::array<std::string_view, 4> kKnownFlags = {
        "device",
        "vk_validation",
        "auto_exit_seconds",
        "screenshot_on_exit"};
    for (const auto flag_name : kKnownFlags)
    {
        const std::string short_prefix = std::string("-") + std::string(flag_name);
        const std::string slash_prefix = std::string("/") + std::string(flag_name);
        if (arg == short_prefix || arg == slash_prefix ||
            StartsWith(arg, short_prefix + "=") ||
            StartsWith(arg, slash_prefix + "="))
        {
            return std::string("--") + std::string(arg.substr(1));
        }
    }
    return std::string(arg);
}

std::vector<std::string> NormalizeCommandLineArgs(int argc, char** argv)
{
    std::vector<std::string> normalized_args = {};
    normalized_args.reserve(static_cast<std::size_t>(std::max(argc, 0)));
    for (int i = 0; i < argc; ++i)
    {
        normalized_args.push_back(NormalizeKnownFlag(argv[i]));
    }
    return normalized_args;
}

} // namespace

namespace frame::common
{

Application::Application(std::unique_ptr<frame::WindowInterface> window)
    : window_(std::move(window))
{
    if (!window_)
    {
        throw std::invalid_argument("Application requires a valid window.");
    }
}

Application::Application(
    int argc,
    char** argv,
    glm::uvec2 size,
    DrawingTargetEnum drawing_target)
{
    auto normalized_args = NormalizeCommandLineArgs(argc, argv);
    std::vector<char*> normalized_argv = {};
    normalized_argv.reserve(normalized_args.size());
    for (auto& arg : normalized_args)
    {
        normalized_argv.push_back(arg.data());
    }
    absl::ParseCommandLine(
        static_cast<int>(normalized_argv.size()),
        normalized_argv.data());
    InitializeFromArgs(argc, argv, size, drawing_target);
}

frame::WindowInterface& Application::GetWindow()
{
    if (!window_)
    {
        throw std::runtime_error("Application window not initialized.");
    }
    return *window_;
}

void Application::Startup(std::filesystem::path path)
{
    auto& device = GetWindow().GetDevice();
    GetWindow().SetOpenFileName(path.filename().string());
    if (!plugin_name_.empty())
    {
        device.RemovePluginByName(plugin_name_);
    }
    auto plugin = std::make_unique<Draw>(GetWindow().GetSize(), path, device);
    plugin_name_ = "ApplicationDraw";
    plugin->SetName(plugin_name_);
    device.AddPlugin(std::move(plugin));
}

void Application::Startup(std::unique_ptr<frame::LevelInterface> level)
{
    auto& device = GetWindow().GetDevice();
    GetWindow().SetOpenFileName("");
    if (!plugin_name_.empty())
    {
        device.RemovePluginByName(plugin_name_);
    }
    auto plugin =
        std::make_unique<Draw>(GetWindow().GetSize(), std::move(level), device);
    plugin_name_ = "ApplicationDraw";
    plugin->SetName(plugin_name_);
    device.AddPlugin(std::move(plugin));
}

void Application::Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum)
{
    GetWindow().Resize(size, fullscreen_enum);
}

WindowReturnEnum Application::Run(std::function<bool()> lambda)
{
    const double auto_exit_seconds = absl::GetFlag(FLAGS_auto_exit_seconds);
    if (auto_exit_seconds <= 0.0)
    {
        return GetWindow().Run(std::move(lambda));
    }

    auto& logger = frame::Logger::GetInstance();
    const auto start = std::chrono::steady_clock::now();
    bool auto_exit_logged = false;
    return GetWindow().Run(
        [lambda = std::move(lambda),
         auto_exit_seconds,
         start,
         this,
         &logger,
         auto_exit_logged]() mutable {
            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<double> elapsed = now - start;
            const bool keep_running = elapsed.count() < auto_exit_seconds;
            if (!keep_running && !auto_exit_logged)
            {
                if (absl::GetFlag(FLAGS_screenshot_on_exit))
                {
                    try
                    {
                        GetWindow().GetDevice().ScreenShot("ScreenShot.png");
                    }
                    catch (const std::exception& ex)
                    {
                        logger->warn(
                            "Failed to save screenshot on exit: {}",
                            ex.what());
                    }
                }
                logger->info(
                    "Auto exit triggered after {:.3f} seconds.",
                    auto_exit_seconds);
                logger->flush();
                auto_exit_logged = true;
            }
            return keep_running && lambda();
        });
}

RenderingAPIEnum Application::ParseDeviceFlag(const std::string& value) const
{
    const std::string lowered = absl::AsciiStrToLower(value);
    if (lowered == "opengl")
    {
        return RenderingAPIEnum::OPENGL;
    }
    if (lowered == "vulkan")
    {
        return RenderingAPIEnum::VULKAN;
    }
    frame::Logger::GetInstance()->warn(
        "Unknown rendering device '{}', defaulting to Vulkan.",
        value);
    return RenderingAPIEnum::VULKAN;
}

std::unique_ptr<frame::WindowInterface> Application::CreateWindowOrThrow(
    DrawingTargetEnum drawing_target,
    RenderingAPIEnum api,
    glm::uvec2 size) const
{
    if (api == RenderingAPIEnum::VULKAN)
    {
        frame::vulkan::EnsureWindowFactoryRegistered();
    }
    auto window = frame::CreateNewWindow(drawing_target, api, size);
    if (!window)
    {
        throw std::runtime_error("Failed to create rendering window.");
    }
    return window;
}

void Application::InitializeFromArgs(
    int /*argc*/,
    char** /*argv*/,
    glm::uvec2 size,
    DrawingTargetEnum drawing_target)
{
    const auto requested = ParseDeviceFlag(absl::GetFlag(FLAGS_device));

    auto attempt = [&](RenderingAPIEnum api) {
        return CreateWindowOrThrow(drawing_target, api, size);
    };

    if (requested == RenderingAPIEnum::VULKAN)
    {
        try
        {
            window_ = attempt(RenderingAPIEnum::VULKAN);
            return;
        }
        catch (const std::exception& ex)
        {
            frame::Logger::GetInstance()->warn(
                "Vulkan startup failed, falling back to OpenGL: {}",
                ex.what());
        }
    }

    window_ = attempt(
        requested == RenderingAPIEnum::VULKAN
            ? RenderingAPIEnum::OPENGL
            : requested);
}

} // namespace frame::common
