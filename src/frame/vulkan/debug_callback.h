#pragma once

#include <vulkan/vulkan.hpp>

namespace frame::vulkan {

	VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
		void* p_user_data);

	// CHECKME(anirul): Is this really needed?
	VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugReportCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		size_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData);

}  // End namespace frame::vulkan.
