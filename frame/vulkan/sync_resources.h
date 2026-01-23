#pragma once

#include <cstddef>
#include <vector>

#include "frame/vulkan/vulkan_dispatch.h"

namespace frame::vulkan
{

class SyncResources
{
  public:
    SyncResources(vk::Device device, std::size_t max_frames_in_flight);

    void Create();
    void Destroy();

    bool IsCreated() const
    {
        return created_;
    }

    std::size_t GetFrameCount() const
    {
        return frame_count_;
    }

    vk::Semaphore GetImageAvailable(std::size_t frame) const
    {
        return *image_available_.at(frame);
    }

    vk::Semaphore GetRenderFinished(std::size_t frame) const
    {
        return *render_finished_.at(frame);
    }

    vk::Fence GetInFlightFence(std::size_t frame) const
    {
        return *in_flight_.at(frame);
    }

  private:
    vk::Device device_;
    std::size_t frame_count_ = 0;
    bool created_ = false;
    std::vector<vk::UniqueSemaphore> image_available_;
    std::vector<vk::UniqueSemaphore> render_finished_;
    std::vector<vk::UniqueFence> in_flight_;
};

} // namespace frame::vulkan
