#include "frame/gui/draw_gui_factory.h"

#include "frame/opengl/gui/sdl_opengl_draw_gui.h"

namespace frame::gui {

std::unique_ptr<frame::gui::DrawGuiInterface> CreateDrawGui(DeviceInterface* device,
                                                            WindowInterface* window) {
    switch (device->GetDeviceEnum()) {
        case RenderingAPIEnum::OPENGL:
            return std::make_unique<frame::opengl::gui::SDL2OpenGLDrawGui>(device, window);
        default:
            return nullptr;
    }
}

}  // End namespace frame::gui.
