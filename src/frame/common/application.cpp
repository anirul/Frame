#include "frame/common/application.h"

#include "frame/common/draw.h"

namespace frame::common
{

Application::Application(std::unique_ptr<frame::WindowInterface> &&window)
    : window_(std::move(window))
{
    assert(window_);
}

void Application::Startup(std::filesystem::path path)
{
    assert(window_);
    auto &device = window_->GetDevice();
    if (!plugin_name_.empty())
    {
        device.RemovePluginByName(plugin_name_);
    }
    auto plugin = std::make_unique<Draw>(window_->GetSize(), path, device);
    plugin_name_ = "ApplicationDraw";
    plugin->SetName(plugin_name_);
    device.AddPlugin(std::move(plugin));
}

void Application::Startup(std::unique_ptr<frame::LevelInterface> &&level)
{
    assert(window_);
    auto &device = window_->GetDevice();
    if (!plugin_name_.empty())
    {
        device.RemovePluginByName(plugin_name_);
    }
    auto plugin =
        std::make_unique<Draw>(window_->GetSize(), std::move(level), device);
    plugin_name_ = "ApplicationDraw";
    plugin->SetName(plugin_name_);
    device.AddPlugin(std::move(plugin));
}

void Application::Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum)
{
    assert(window_);
    window_->Resize(size, fullscreen_enum);
}

void Application::Run(std::function<void()> lambda)
{
    assert(window_);
    window_->Run(lambda);
}

} // End namespace frame::common.
