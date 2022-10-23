#pragma once

#include <filesystem>
#include <memory>

#include "frame/api.h"
#include "frame/common/path_interface.h"
#include "frame/window_interface.h"
#include "frame/proto/level.pb.h"

namespace frame::common {

/**
 * @class Application
 * @brief This class is wrapping all the inner working of a window inside a container.
 */
class Application {
   public:
    /**
     * @brief Construct a new Application object.
     * @param window: The unique pointer to the window (should have been created prior).
     */
    Application(std::unique_ptr<frame::WindowInterface>&& window);
    /**
     * @brief Startup this will initialized the inner level of the window according to a path.
     * @param path: The path to the JSON file that describe the inner working of the window.
     */
    void Startup(std::filesystem::path path);
    /**
     * @brief Same as previously but this use a level.
     * @param level: A unique pointer to a level.
     */
    void Startup(std::unique_ptr<frame::LevelInterface>&& level);
    /**
     * @brief A helper function that call the inner resize of the window.
     * @param size: The new size of the window.
     */
    void Resize(std::pair<std::uint32_t, std::uint32_t> size);
    /**
     * @brief A helper function that call the inner full screen of the window.
     * @param fullscreen: The new full screen state of the window.
     */
    void SetFullscreen(frame::FullScreenEnum fullscreen_mode);
    /**
     * @brief Run the application.
     */
    void Run();

   protected:
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
    int index_                                      = -1;
};

}  // End namespace frame::common.
