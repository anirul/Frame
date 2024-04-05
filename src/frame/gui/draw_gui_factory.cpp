#include "frame/gui/draw_gui_factory.h"

#include "frame/opengl/gui/sdl_opengl_draw_gui.h"

namespace frame::gui
{

std::unique_ptr<frame::gui::DrawGuiInterface> CreateDrawGui(
    WindowInterface& window, const std::filesystem::path& font_path, float font_size)
{
    auto& device = window.GetDevice();
    switch (device.GetDeviceEnum())
    {
    case RenderingAPIEnum::OPENGL:
        return std::make_unique<frame::opengl::gui::SDL2OpenGLDrawGui>(
            window, font_path, font_size);
    default:
        return nullptr;
    }
}

} // End namespace frame::gui.
