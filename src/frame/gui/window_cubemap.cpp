#include "frame/gui/window_cubemap.h"

#include <imgui.h>

#include <filesystem>

#include "frame/file/file_system.h"
#include "frame/opengl/texture_cube_map.h"
#include "frame/level_interface.h"

namespace frame::gui
{

WindowCubemap::WindowCubemap(TextureInterface& texture_interface)
    : name_(""), size_(0, 0), texture_interface_(texture_interface)
{
    if (!texture_interface_.IsCubeMap())
    {
        throw std::runtime_error("Cannot create a normal texture!");
    }
    name_ = std::format("cubemap - [{}]", texture_interface_.GetName());
}

bool WindowCubemap::DrawCallback()
{
    frame::opengl::TextureCubeMap& texture_cubemap =
        dynamic_cast<frame::opengl::TextureCubeMap&>(texture_interface_);

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
