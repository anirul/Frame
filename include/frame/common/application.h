#pragma once

#include <filesystem>
#include <memory>

#include "frame/api.h"
#include "frame/window_interface.h"

namespace frame::common
{

/**
 * @class Application
 * @brief This class is wrapping all the inner working of a window inside a
 *        container.
 */
class Application
{
  public:
    /**
     * @brief Construct a new Application object.
     * @param window: The unique pointer to the window (should have been
     *        created prior).
     */
    Application(std::unique_ptr<frame::WindowInterface> window);
    /**
     * @brief Get the Window object.
     * @return The window.
     */
    frame::WindowInterface& GetWindow();
    /**
     * @brief Startup this will initialized the inner level of the window
     *        according to a path.
     * @param path: The path to the JSON file that describe the inner
     *        working of the window.
     */
    void Startup(std::filesystem::path path);
    /**
     * @brief Same as previously but this use a level.
     * @param level: A unique pointer to a level.
     */
    void Startup(std::unique_ptr<frame::LevelInterface> level);
    /**
     * @brief A helper function that call the inner resize of the window.
     * @param size: The new size of the window.
     * @param fullscreen_enum: The new full screen state of the window.
     */
    void Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum);
    /**
     * @brief Run the application.
     */
    void Run(std::function<void()> lambda = [] {});

  protected:
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
    std::string plugin_name_;
};

} // End namespace frame::common.
