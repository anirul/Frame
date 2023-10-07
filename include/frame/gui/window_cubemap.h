#pragma once

#include <frame/gui/draw_gui_interface.h>
#include <frame/level_interface.h>

#include <filesystem>

namespace frame::gui
{

class WindowCubemap : public GuiWindowInterface
{
  public:
    /**
     * @brief Constructor
     * @param name: The name of the window.
     */
    WindowCubemap(const std::string& name);

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
    /**
     * @brief Check if this is the end of the software.
     * @return True if this is the end false if not.
     */
    bool End() const override;
    /**
     * @brief Change the sky-box cube map path.
     * @param level: The level where to change the cube map path.
     */
    void ChangeLevel(frame::proto::Level& level);

  protected:
    /**
     * @brief Browse the asset/cubemap directory to get the cube maps list.
     * @return A list of the cube maps.
     */
    std::vector<std::filesystem::path> GetCubemaps();
    /**
     * @brief Load a cube map to a level.
     * @param path: The path of the cube map.
     */
    void SaveCubemapPath(const std::filesystem::path& path);

  private:
    std::string name_;
    std::filesystem::path cubemap_path_;
    bool end_ = true;
};

} // End namespace frame::gui.
