#pragma once

#include <memory>
#include <filesystem>

#include "frame/device_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/window_interface.h"

namespace frame::gui
{

std::unique_ptr<DrawGuiInterface> CreateDrawGui(
    WindowInterface& window,
	const std::filesystem::path& font_path,
	float font_size);

} // End namespace frame::gui.
