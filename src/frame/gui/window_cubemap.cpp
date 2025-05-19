#include "frame/gui/window_cubemap.h"

#include <imgui.h>

#include <filesystem>

#include "frame/file/file_system.h"
#include "frame/opengl/cubemap.h"
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
    name_ = std::format("cubemap - [{}]", texture_interface_.GetName());
}

bool WindowCubemap::DrawCallback()
{
    opengl::Cubemap& texture_cubemap =
        dynamic_cast<opengl::Cubemap&>(texture_interface_);

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
