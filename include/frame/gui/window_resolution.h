#pragma once

#include <string>

#include "frame/api.h"
#include "frame/gui/draw_gui_interface.h"

namespace frame::gui {

/**
 * @class GuiWindowInterface
 * @brief Select to change resolution window.
 */
class WindowResolution : public GuiWindowInterface {
   public:
    /**
     * @brief Default constructor.
     * @param name: The name of the window.
     * @param size: The initial size of the window.
     */
    WindowResolution(const std::string& name, std::pair<std::uint32_t, std::uint32_t> size,
                     std::pair<std::uint32_t, std::uint32_t> border_less_size);
    //! @brief Virtual destructor.
    virtual ~WindowResolution() = default;

   public:
    //! @brief Draw callback setting.
    bool DrawCallback() override;
    /**
     * @brief Get the name of the window.
     * @return The name of the window.
     */
    std::string GetName() const override;
    /**
     * @brief Set the name of the window.
     * @param name: The name of the window.
     */
    void SetName(const std::string& name) override;

   public:
    /**
     * @brief Get the window size.
	 * This will vary according to what full screen mode is selected!
     * @return The size of the window.
     */
    std::pair<std::uint32_t, std::uint32_t> GetSize() const;
    /**
     * @brief Get the full screen mode.
     * @return The full screen mode.
     */
    FullScreenEnum GetFullScreen() const { return fullscreen_; }
    /**
     * @brief Check if this is the end of the software.
     * @return True if this is the end false if not.
     */
    bool End();

   private:
    std::pair<std::uint32_t, std::uint32_t> size_;
    std::pair<std::uint32_t, std::uint32_t> border_less_size_;
    FullScreenEnum fullscreen_ = FullScreenEnum::WINDOW;
    struct ResolutionElement {
        std::pair<std::uint32_t, std::uint32_t> values;
        std::string name;
    };
    std::vector<std::string> resolution_items_;
    std::vector<std::string> fullscreen_items_;
    const std::vector<ResolutionElement> resolutions_ = {
        { { 640, 480 }, "VGA" },        { { 800, 600 }, "SVGA" },
        { { 1024, 768 }, "XGA" },       { { 1280, 720 }, "WXGA-H" },
        { { 1920, 1080 }, "FHD" },      { { 2560, 1080 }, "UW-FHD" },
        { { 2560, 1440 }, "QHD" },      { { 3440, 1440 }, "UW-QHD" },
        { { 3840, 2160 }, "4K UHD-1" }, { { 7680, 4320 }, "8K UHD-2" },
    };
    const std::vector<FullScreenEnum> fullscreen_mode_ = {
        FullScreenEnum::WINDOW,
        FullScreenEnum::FULLSCREEN,
        FullScreenEnum::FULLSCREEN_DESKTOP,
    };
    int resolution_selected_ = 0;
    int fullscreen_selected_ = 0;
    std::string name_;
    bool end_ = true;
};

}  // End namespace frame::gui.
