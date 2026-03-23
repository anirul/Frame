#pragma once

#include "frame/vulkan/vulkan_dispatch.h"

namespace frame::vulkan
{

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data);

// CHECKME(anirul): Is this really needed?
VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugReportCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT object_type,
    uint64_t object,
    size_t location,
    int32_t message_code,
    const char* layer_prefix,
    const char* message,
    void* user_data);

} // namespace frame::vulkan
