#include "debug_callback.h"

namespace frame::vulkan {

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objectType,
                                                   uint64_t object, size_t location,
                                                   int32_t messageCode, const char* pLayerPrefix,
                                                   const char* pMessage, void* pUserData) {
    // TODO(anirul): Improve this.
    throw std::runtime_error(pMessage);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
              VkDebugUtilsMessageTypeFlagsEXT message_type,
              const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data) {
    // TODO(anirul): Improve this.
    throw std::runtime_error(p_callback_data->pMessage);
}

}  // namespace frame::vulkan
