#include "frame/gui/window_texture.h"

#include <SDL3/SDL.h>
#include <format>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include "frame/opengl/texture.h"

namespace frame::gui
{

WindowTexture::WindowTexture(TextureInterface& texture_interface)
    : name_(""), size_(0, 0), texture_interface_(texture_interface)
{
    if (texture_interface_.GetData().cubemap())
    {
        throw std::runtime_error(
            "Cannot create a window for a cubemap texture!");
    }
    name_ = std::format(
        "texture - [{}] - ({}, {})",
        texture_interface_.GetName(),
        texture_interface_.GetSize().x,
        texture_interface_.GetSize().y);
}

bool WindowTexture::DrawCallback()
{
    frame::opengl::Texture& texture =
        dynamic_cast<frame::opengl::Texture&>(texture_interface_);

    // Get the available content region (assuming zero window padding)
    ImVec2 content_window = ImGui::GetContentRegionAvail();

    // Compute the final size (window_range) as you already do.
    auto texture_size = texture.GetSize();
    float aspect_ratio = static_cast<float>(texture_size.x) /
                         static_cast<float>(texture_size.y);
    ImTextureID gl_id = static_cast<ImTextureID>(texture.GetId());
    ImVec2 window_range{};
    if (content_window.x / aspect_ratio > content_window.y)
    {
        window_range =
            ImVec2(content_window.y * aspect_ratio, content_window.y);
    }
    else
    {
        window_range =
            ImVec2(content_window.x, content_window.x / aspect_ratio);
    }

    // Compute offset for centering:
    ImVec2 image_offset;
    image_offset.x = (content_window.x - window_range.x) * 0.5f;
    image_offset.y = (content_window.y - window_range.y) * 0.5f;

    // Set the cursor position to the computed offset.
    ImGui::SetCursorPos(image_offset);

    // Draw the image.
    ImGui::Image(gl_id, window_range, ImVec2(0, 1), ImVec2(1, 0));

    return true;
}

bool WindowTexture::End() const
{
    return false;
}

std::string WindowTexture::GetName() const
{
    return name_;
}

void WindowTexture::SetName(const std::string& name)
{
    name_ = name;
}

} // End namespace frame::gui.
