#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include "absl/flags/declare.h"

#include "frame/api.h"
#include "frame/window_interface.h"

ABSL_DECLARE_FLAG(bool, vk_validation);

namespace frame::common
{

/**
 * @class Application
 * @brief Wrapper owning a rendering window and its lifecycle.
 */
class Application
{
  public:
    Application(std::unique_ptr<frame::WindowInterface> window);
    Application(
        int argc,
        char** argv,
        glm::uvec2 size,
        DrawingTargetEnum drawing_target = DrawingTargetEnum::WINDOW);

    frame::WindowInterface& GetWindow();
    void Startup(std::filesystem::path path);
    void Startup(std::unique_ptr<frame::LevelInterface> level);
    void Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum);
    WindowReturnEnum Run(std::function<bool()> lambda = [] { return true; });

  private:
    RenderingAPIEnum ParseDeviceFlag(const std::string& value) const;
    std::unique_ptr<frame::WindowInterface> CreateWindowOrThrow(
        DrawingTargetEnum drawing_target,
        RenderingAPIEnum api,
        glm::uvec2 size) const;
    void InitializeFromArgs(
        int argc,
        char** argv,
        glm::uvec2 size,
        DrawingTargetEnum drawing_target);

  protected:
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
    std::string plugin_name_;
};

} // End namespace frame::common.
