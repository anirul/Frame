#pragma once

#include <filesystem>

#include "frame/common/path_interface.h"
#include "frame/device_interface.h"
#include "frame/logger.h"
#include "frame/plugin_interface.h"

namespace frame::common {

/**
 * @class Draw
 * @brief The draw class is the main class that will be used to draw the level.
 */
class Draw : public frame::PluginInterface {
   public:
    /**
     * @brief Construct a new Draw object.
     * @param size: The size of the window.
     * @param path: A path to the JSON file containing the level interface.
     * @param device: A pointer to the current device (come from the window).
     */
    Draw(glm::uvec2 size, std::filesystem::path path, frame::DeviceInterface* device)
        : size_(size), draw_type_based_(DrawTypeEnum::PATH), path_(path), device_(device) {}
    /**
     * @brief Construct a new Draw object.
     * @param size: The size of the window.
     * @param level: A unique pointer to the level interface.
     * @param device: A pointer to the current device (come from the window).
     */
    Draw(glm::uvec2 size, std::unique_ptr<frame::LevelInterface>&& level,
         frame::DeviceInterface* device)
        : size_(size),
          draw_type_based_(DrawTypeEnum::LEVEL),
          level_(std::move(level)),
          device_(device) {}

   public:
    /**
     * @brief Startup the draw.
     * @param size: The size of the draw place.
     */
    void Startup(glm::uvec2 size) override;
    /**
     * @brief Called before rendering.
     * @param uniform[in, out]: The uniform data.
     * @param device: The device.
     * @param level: The level.
     * @param static_mesh: The static mesh.
     * @param material: The material associated with the mesh.
     */
    void PreRender(UniformInterface& uniform, DeviceInterface* device,
                   StaticMeshInterface* static_mesh, MaterialInterface* material) override;
    /**
     * @brief Called to update variables, called after the main render phase.
     * @param device: The device.
     * @param dt: Delta time between previous frame and present frame.
     * @return true: If the draw is still running.
     */
    bool Update(DeviceInterface* device, double dt = 0.0) override;

   public:
    /**
     * @brief Get name.
     * @return Name.
     */
    std::string GetName() const override { return name_; }
    /**
     * @brief Set name.
     * @return Name.
     */
    void SetName(const std::string& name) override { name_ = name; }
    /**
     * @brief Get the event from the window interface and pass them to the draw.
     * @param event: The event to be passed to the draw.
     * @return Was it used or not.
     */
    bool PollEvent(void* event) override { return false; }
    //! @brief Called to cleanup at the end.
    void End() override {}

   private:
    frame::Logger& logger_ = frame::Logger::GetInstance();
    glm::uvec2 size_       = { 0, 0 };
    enum class DrawTypeEnum {
        PATH,
        LEVEL,
    };
    const DrawTypeEnum draw_type_based_;
    std::filesystem::path path_;
    std::unique_ptr<frame::LevelInterface> level_;
    frame::DeviceInterface* device_ = nullptr;
    std::string name_;
    double dt_ = 0.0;
};

}  // End namespace frame::common.
