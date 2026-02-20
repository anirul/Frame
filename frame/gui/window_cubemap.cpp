#include "frame/gui/window_cubemap.h"

#include <imgui.h>

#include <filesystem>

#include "frame/file/file_system.h"
#include "frame/opengl/cubemap.h"
#include "frame/opengl/texture.h"
#include "frame/level_interface.h"

namespace frame::gui
{

WindowCubemap::WindowCubemap(TextureInterface& texture_interface)
    : name_(""), size_(0, 0), texture_interface_(texture_interface)
{
    if (!texture_interface_.GetData().cubemap())
    {
        throw std::runtime_error("Cannot create a normal texture!");
    }
    auto display_size = texture_interface_.GetSize();
    name_ = std::format(
        "cubemap - [{}] - ({}, {})",
        texture_interface_.GetName(),
        display_size.x,
        display_size.y);

    auto* texture_cubemap = dynamic_cast<opengl::Cubemap*>(&texture_interface_);
    if (!texture_cubemap)
    {
        return;
    }

    auto pixel_size = texture_cubemap->GetData().pixel_element_size();
    auto pixel_struct = texture_cubemap->GetData().pixel_structure();
    auto face_size = texture_cubemap->GetSize();
    std::uint32_t channels = 3;
    switch (pixel_struct.value())
    {
    case proto::PixelStructure::GREY:
    case proto::PixelStructure::DEPTH:
        channels = 1;
        break;
    case proto::PixelStructure::GREY_ALPHA:
        channels = 2;
        break;
    case proto::PixelStructure::RGB:
    case proto::PixelStructure::BGR:
        channels = 3;
        break;
    case proto::PixelStructure::RGB_ALPHA:
    case proto::PixelStructure::BGR_ALPHA:
        channels = 4;
        break;
    default:
        break;
    }

    std::size_t slice_pixels =
        static_cast<std::size_t>(face_size.x) * face_size.y * channels;

    if (pixel_size.value() == proto::PixelElementSize::BYTE)
    {
        auto data = texture_cubemap->GetTextureByte();
        for (int i = 0; i < 6; ++i)
        {
            face_textures_[i] = std::make_unique<opengl::Texture>(
                data.data() + i * slice_pixels,
                face_size,
                pixel_size,
                pixel_struct);
        }
    }
    else if (
        pixel_size.value() == proto::PixelElementSize::SHORT ||
        pixel_size.value() == proto::PixelElementSize::HALF)
    {
        auto data = texture_cubemap->GetTextureWord();
        for (int i = 0; i < 6; ++i)
        {
            face_textures_[i] = std::make_unique<opengl::Texture>(
                data.data() + i * slice_pixels,
                face_size,
                pixel_size,
                pixel_struct);
        }
    }
    else if (pixel_size.value() == proto::PixelElementSize::FLOAT)
    {
        auto data = texture_cubemap->GetTextureFloat();
        for (int i = 0; i < 6; ++i)
        {
            face_textures_[i] = std::make_unique<opengl::Texture>(
                data.data() + i * slice_pixels,
                face_size,
                pixel_size,
                pixel_struct);
        }
    }
}

bool WindowCubemap::DrawCallback()
{
    if (!face_textures_[0])
    {
        ImGui::TextUnformatted(
            "Cubemap preview sub-windows currently support OpenGL textures only.");
        return true;
    }

    ImVec2 content = ImGui::GetContentRegionAvail();
    float cell_w = content.x / 3.0f;
    float cell_h = content.y / 2.0f;
    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            int idx = row * 3 + col;
            if (face_textures_[idx])
            {
                ImGui::Image(
                    static_cast<ImTextureID>(face_textures_[idx]->GetId()),
                    ImVec2(cell_w, cell_h),
                    ImVec2(0, 1),
                    ImVec2(1, 0));
            }
            if (col < 2)
                ImGui::SameLine();
        }
    }
    return true;
}

std::string WindowCubemap::GetName() const
{
    return name_;
}

void WindowCubemap::SetName(const std::string& name)
{
    name_ = name;
}

bool WindowCubemap::End() const
{
    return false;
}

} // End namespace frame::gui.
