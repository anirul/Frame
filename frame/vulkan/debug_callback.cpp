#include "frame/vulkan/debug_callback.h"

#include "frame/logger.h"

namespace frame::vulkan
{

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char* pLayerPrefix,
    const char* pMessage,
    void* pUserData)
{
    (void)flags;
    (void)objectType;
    (void)object;
    (void)location;
    (void)messageCode;
    (void)pLayerPrefix;
    (void)pUserData;
    auto& logger = Logger::GetInstance();
    logger->warn(pMessage ? pMessage : "Vulkan debug report message.");
    return VK_FALSE;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data)
{
    auto& logger = Logger::GetInstance();
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        logger->error(p_callback_data->pMessage);
        return VK_FALSE;
    }
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        logger->info(p_callback_data->pMessage);
    }
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        logger->warn(p_callback_data->pMessage);
    }
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        logger->info(p_callback_data->pMessage);
        return VK_FALSE;
    }
    return VK_FALSE;
}

} // namespace frame::vulkan
