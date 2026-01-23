#include "frame/vulkan/sync_resources.h"

namespace frame::vulkan
{

SyncResources::SyncResources(vk::Device device, std::size_t max_frames_in_flight)
    : device_(device),
      frame_count_(max_frames_in_flight)
{
}

void SyncResources::Create()
{
    if (created_)
    {
        return;
    }

    image_available_.resize(frame_count_);
    render_finished_.resize(frame_count_);
    in_flight_.resize(frame_count_);

    for (std::size_t i = 0; i < frame_count_; ++i)
    {
        image_available_[i] = device_.createSemaphoreUnique({});
        render_finished_[i] = device_.createSemaphoreUnique({});
        vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);
        in_flight_[i] = device_.createFenceUnique(fence_info);
    }

    created_ = true;
}

void SyncResources::Destroy()
{
    image_available_.clear();
    render_finished_.clear();
    in_flight_.clear();
    created_ = false;
}

} // namespace frame::vulkan
