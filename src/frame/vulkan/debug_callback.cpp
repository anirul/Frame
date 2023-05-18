#include "debug_callback.h"

#include "frame/logger.h"

namespace frame::vulkan {

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
    uint64_t object, size_t location, int32_t messageCode,
    const char* pLayerPrefix, const char* pMessage, void* pUserData) {
  // TODO(anirul): Improve this.
  throw std::runtime_error(pMessage);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
              VkDebugUtilsMessageTypeFlagsEXT message_type,
              const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
              void* p_user_data) {
  if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    throw std::runtime_error(p_callback_data->pMessage);
  }
  auto& logger = Logger::GetInstance();
  if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    logger->info(p_callback_data->pMessage);
  }
  if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    logger->warn(p_callback_data->pMessage);
  }
  if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    logger->info(p_callback_data->pMessage);
    return VK_FALSE;
  }
  return VK_TRUE;
}

}  // namespace frame::vulkan
