#pragma once

#include "frame/vulkan/vulkan_dispatch.h"

namespace frame::vulkan
{

class Device;
struct SceneState;

class RaytraceSceneRenderer
{
  public:
    explicit RaytraceSceneRenderer(Device& device);

    void UpdateUniformBuffer(const SceneState& state);
    void Render(vk::CommandBuffer command_buffer, vk::Extent2D extent);

  private:
    Device& device_;
};

} // namespace frame::vulkan
