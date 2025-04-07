#include "view_windows.h"

#include <SDL3/SDL.h>
#include <format>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include "frame/gui/window_logger.h"
#include "frame/opengl/texture.h"
#include "window_texture.h"

namespace frame::gui
{

ViewWindows::ViewWindows(
    DrawGuiInterface* draw_gui,
    glm::uvec2 size,
    glm::uvec2 desktop_size,
    glm::uvec2 pixel_per_inch)
    : draw_gui_(draw_gui),
      size_(size),
      desktop_size_(desktop_size),
      pixel_per_inch_(pixel_per_inch)
{
}

void ViewWindows::CreateLogger(const std::string& name)
{
    draw_gui_->AddWindow(std::make_unique<WindowLogger>(name));
}

void ViewWindows::DeleteLogger(const std::string& name)
{
    draw_gui_->DeleteWindow(name);
}

void ViewWindows::CreateResolution(const std::string& name)
{
    std::unique_ptr<WindowResolution> unique_window_resolution =
        std::make_unique<WindowResolution>(
            name,
            size_,
            desktop_size_,
            pixel_per_inch_);
    ptr_window_resolution_ = unique_window_resolution.get();
    draw_gui_->AddWindow(std::move(unique_window_resolution));
}

void ViewWindows::DeleteResolution(const std::string& name)
{
    ptr_window_resolution_ = nullptr;
    draw_gui_->DeleteWindow(name);
}

void ViewWindows::ShowLoggerWindow()
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

void ViewWindows::ShowResolutionWindow()
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

void ViewWindows::ShowTexturesWindow(DeviceInterface& device)
{
    auto& level = device.GetLevel();
    for (auto id : level.GetTextures())
    {
        frame::TextureInterface& texture_interface =
            device.GetLevel().GetTextureFromId(id);
        std::string str_type =
            texture_interface.IsCubeMap() ? "cubemap" : "texture";
        if (!window_state_.contains(texture_interface.GetName()))
        {
            window_state_[texture_interface.GetName()] = false;
        }
        if (ImGui::MenuItem(
                std::format(
                    "{} - [{}]",
                    str_type,
                    texture_interface.GetName(), id).c_str(),
                "",
                &window_state_[texture_interface.GetName()]))
        {
            if (window_state_[texture_interface.GetName()])
            {
                draw_gui_->AddWindow(
                    std::make_unique<WindowTexture>(texture_interface));
            }
            else
            {
                draw_gui_->DeleteWindow(
                    std::format(
                        "{} - [{}]", str_type, texture_interface.GetName()));
            }
        }
    }
}

} // End namespace frame::gui.
