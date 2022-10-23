#pragma once

#include <filesystem>

#include "frame/common/path_interface.h"
#include "frame/device_interface.h"
#include "frame/draw_interface.h"
#include "frame/logger.h"
#include "frame/opengl/window.h"

namespace frame::common {

/**
 * @class Draw
 * @brief The draw class is the main class that will be used to draw the level.
 */
class Draw : public frame::DrawInterface {
   public:
    /**
     * @brief Construct a new Draw object.
     * @param size: The size of the window.
     * @param path: A path to the JSON file containing the level interface.
     * @param device: A pointer to the current device (come from the window).
     */
    Draw(const std::pair<std::uint32_t, std::uint32_t> size, std::filesystem::path path,
         frame::DeviceInterface* device)
        : size_(size), draw_type_based_(DrawTypeEnum::PATH), path_(path), device_(device) {}
    /**
     * @brief Construct a new Draw object.
     * @param size: The size of the window.
     * @param level: A unique pointer to the level interface.
     * @param device: A pointer to the current device (come from the window).
     */
    Draw(const std::pair<std::uint32_t, std::uint32_t> size,
         std::unique_ptr<frame::LevelInterface>&& level, frame::DeviceInterface* device)
        : size_(size),
          draw_type_based_(DrawTypeEnum::LEVEL),
          level_(std::move(level)),
          device_(device) {}

   public:
    /**
     * @brief Startup the draw.
	 * @param size: The size of the draw place.
     */
    void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
    /**
     * @brief Run the draw (this will be called at each frame).
	 * @param dt: Delta time between frames.
     * @return true: If the draw is still running.
     */
    bool RunDraw(double dt) override;

   public:
    /**
     * @brief Get the event from the window interface and pass them to the draw.
     * @param event: The event to be passed to the draw.
     */
    bool PollEvent(void* event) override { return false; }

   private:
    frame::Logger& logger_                        = frame::Logger::GetInstance();
    std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
    enum class DrawTypeEnum {
        PATH,
        LEVEL,
    };
    const DrawTypeEnum draw_type_based_;
    std::filesystem::path path_;
    std::unique_ptr<frame::LevelInterface> level_;
    frame::DeviceInterface* device_ = nullptr;
};

}  // End namespace frame::common.
