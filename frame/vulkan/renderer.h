#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>

#include "frame/vulkan/vulkan_dispatch.h"

namespace frame::vulkan
{

class Device;
class RaytraceSceneRenderer;

/**
 * @class Renderer
 * @brief Backend-local frame renderer for Vulkan acquire/submit/present work.
 */
class Renderer
{
  public:
    explicit Renderer(Device& device);
    ~Renderer();

    void Display(double dt);
    void Reset();

  private:
    void RecordCommandBuffer(
        vk::CommandBuffer command_buffer,
        std::uint32_t image_index);

  private:
    Device& device_;
    std::size_t current_frame_ = 0;
    std::unique_ptr<RaytraceSceneRenderer> raytrace_scene_renderer_;
};

} // namespace frame::vulkan
