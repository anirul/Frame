#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "frame/window_interface.h"

namespace frame::vulkan
{

struct VulkanWindowInterface : public WindowInterface
{
    virtual vk::InstanceCreateInfo GetInstanceCreateInfo(
        vk::ApplicationInfo app_info) const = 0;
};

} // namespace frame::vulkan
