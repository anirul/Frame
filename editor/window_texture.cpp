#include "window_texture.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <format>

#include "frame/opengl/texture.h"

namespace frame::gui
{

WindowTexture::WindowTexture(
    TextureInterface& texture_interface)
    : texture_interface_(texture_interface)
{
    name_ = std::format("texture - [{}]", texture_interface_.GetName());
}

WindowTexture::~WindowTexture() = default;

bool WindowTexture::DrawCallback()
{
    frame::opengl::Texture& texture =
        dynamic_cast<frame::opengl::Texture&>(texture_interface_);
    ImGui::Begin(std::format("texture - [{}]", name_).c_str());
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
    // Draw the image.
    ImGui::Image(gl_id, window_range, ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
}

} // End namespace frame::gui.
