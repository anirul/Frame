#include "window_texture.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <format>

#include "frame/opengl/texture.h"

namespace frame::gui
{

WindowTexture::WindowTexture(TextureInterface& texture_interface) :
    name_(""),
    size_(0, 0),
    texture_interface_(texture_interface)
{
    if (texture_interface_.IsCubeMap())
    {
        throw std::runtime_error(
            "Cannot create a window for a cubemap texture!");
    }
    name_ = std::format("texture - [{}]", texture_interface_.GetName());
}

bool WindowTexture::DrawCallback()
{
    frame::opengl::Texture& texture =
        dynamic_cast<frame::opengl::Texture&>(texture_interface_);
    // Get the window width.
    ImVec2 content_window = ImGui::GetContentRegionAvail();
    auto size = texture.GetSize();
    // Compute the aspect ratio.
    float aspect_ratio =
        static_cast<float>(size.x) / static_cast<float>(size.y);
    // Cast the opengl windows id.
    // I disable the warning C4312 from unsigned int to void* casting to
    // a bigger space.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push)
#pragma warning(disable : 4312)
#endif
    ImTextureID gl_id = static_cast<ImTextureID>(texture.GetId());
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(pop)
#endif
    // Compute the final size.
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
    ImVec2 window_size = ImGui::GetContentRegionAvail();
    size_ = glm::uvec2(window_size.x, window_size.y);
    // Draw the image.
    ImGui::Image(gl_id, window_range, ImVec2(0, 1), ImVec2(1, 0));
    return true;
}

glm::uvec2 WindowTexture::GetSize() const
{
    return size_;
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
