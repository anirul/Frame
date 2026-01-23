#include "frame/common/application.h"

#include <stdexcept>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/ascii.h"

#include "frame/common/draw.h"
#include "frame/logger.h"
#include "frame/window_factory.h"
#include "frame/vulkan/window_factory.h"

ABSL_FLAG(std::string, device, "vulkan", "Rendering backend (vulkan|opengl).");
#if defined(_DEBUG)
ABSL_FLAG(bool, vk_validation, true, "Enable Vulkan validation layers.");
#else
ABSL_FLAG(bool, vk_validation, false, "Enable Vulkan validation layers.");
#endif

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
    absl::ParseCommandLine(argc, argv);
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
    return GetWindow().Run(std::move(lambda));
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
