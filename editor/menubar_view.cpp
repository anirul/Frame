#include "menubar_view.h"

#include <SDL3/SDL.h>
#include <format>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include "frame/gui/window_cubemap.h"
#include "frame/gui/window_logger.h"
#include "frame/gui/window_texture.h"
#include "frame/opengl/texture.h"

namespace frame::gui
{

MenubarView::MenubarView(
    DeviceInterface& device,
    DrawGuiInterface& draw_gui,
    glm::uvec2 size,
    glm::uvec2 desktop_size,
    glm::uvec2 pixel_per_inch)
    : device_(device), draw_gui_(draw_gui), size_(size),
      desktop_size_(desktop_size), pixel_per_inch_(pixel_per_inch)
{
}

void MenubarView::CreateLogger(const std::string& name)
{
    draw_gui_.AddWindow(std::make_unique<WindowLogger>(name));
}

void MenubarView::DeleteLogger(const std::string& name)
{
    draw_gui_.DeleteWindow(name);
}

void MenubarView::CreateResolution(const std::string& name)
{
    std::unique_ptr<WindowResolution> unique_window_resolution =
        std::make_unique<WindowResolution>(
            name, size_, desktop_size_, pixel_per_inch_);
    ptr_window_resolution_ = unique_window_resolution.get();
    draw_gui_.AddWindow(std::move(unique_window_resolution));
}

void MenubarView::DeleteResolution(const std::string& name)
{
    ptr_window_resolution_ = nullptr;
    draw_gui_.DeleteWindow(name);
}

void MenubarView::ShowLoggerWindow()
{
    if (!window_state_.contains("Logger"))
    {
        window_state_["Logger"] = false;
    }
    if (window_state_["Logger"])
    {
        DeleteLogger("Logger");
    }
    else
    {
        CreateLogger("Logger");
    }
    window_state_["Logger"] = !window_state_["Logger"];
}

void MenubarView::ShowResolutionWindow()
{
    if (!window_state_.contains("Resolution"))
    {
        window_state_["Resolution"] = false;
    }
    if (window_state_["Resolution"])
    {
        DeleteResolution("Resolution");
    }
    else
    {
        CreateResolution("Resolution");
    }
    window_state_["Resolution"] = !window_state_["Resolution"];
}

void MenubarView::ShowTexturesWindow(DeviceInterface& device)
{
    auto& level = device.GetLevel();
    auto open_windows = draw_gui_.GetWindowTitles();
    for (auto id : level.GetTextures())
    {
        frame::TextureInterface& texture_interface =
            device.GetLevel().GetTextureFromId(id);
        std::string str_type =
            texture_interface.GetData().cubemap() ? "cubemap" : "texture";
        std::string window_name = std::format(
            "{} - [{}] - ({}, {})",
            str_type,
            texture_interface.GetName(),
            texture_interface.GetSize().x,
            texture_interface.GetSize().y);

        bool is_open =
            std::find(open_windows.begin(), open_windows.end(), window_name) !=
            open_windows.end();
        window_state_[texture_interface.GetName()] = is_open;

        bool previous_state = window_state_[texture_interface.GetName()];
        if (ImGui::MenuItem(
                window_name.c_str(),
                "",
                &window_state_[texture_interface.GetName()]))
        {
            if (window_state_[texture_interface.GetName()] && !previous_state)
            {
                if (texture_interface.GetData().cubemap())
                {
                    draw_gui_.AddWindow(
                        std::make_unique<WindowCubemap>(texture_interface));
                }
                else
                {
                    draw_gui_.AddWindow(
                        std::make_unique<WindowTexture>(texture_interface));
                }
            }
            else if (
                !window_state_[texture_interface.GetName()] && previous_state)
            {
                draw_gui_.DeleteWindow(window_name);
            }
        }
    }
}

void MenubarView::Reset()
{
    for (std::string window_name : draw_gui_.GetWindowTitles())
    {
        if (window_name.starts_with("texture - ") ||
            window_name.starts_with("cubemap - "))
        {
            try
            {
                if (window_name.starts_with("cubemap - "))
                {
                    WindowCubemap& window_cubemap =
                        dynamic_cast<WindowCubemap&>(
                            draw_gui_.GetWindow(window_name));
                    draw_gui_.DeleteWindow(window_name);
                }
                else
                {
                    WindowTexture& window_texture =
                        dynamic_cast<WindowTexture&>(
                            draw_gui_.GetWindow(window_name));
                    draw_gui_.DeleteWindow(window_name);
                }
                auto maybe_name = ExtractStringBracket(window_name);
                if (maybe_name)
                {
                    window_state_[maybe_name.value()] = false;
                }
                else
                {
                    frame::Logger::GetInstance()->error(
                        std::format(
                            "Couldn't parse the string {}.", window_name));
                }
            }
            catch (const std::bad_cast& ex)
            {
                frame::Logger::GetInstance()->warn(
                    std::format(
                        "Counldn't cast window named {} to a window display "
                        "{}.",
                        window_name,
                        ex.what()));
            }
        }
    }
}

std::optional<std::string> MenubarView::ExtractStringBracket(
    const std::string& str) const
{
    auto open = str.find('[');
    auto close = str.find(']', open == std::string::npos ? 0 : open);
    if (open != std::string::npos && close != std::string::npos && close > open)
    {
        return str.substr(open + 1, close - open - 1);
    }
    return std::nullopt;
}

} // End namespace frame::gui.
