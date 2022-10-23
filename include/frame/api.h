#pragma once

namespace frame {

/**
 * @class DeviceEnum
 * @brief List of Device API.
 */
enum class DeviceEnum {
    OPENGL,
    VULKAN,
#if defined(_WIN32) || defined(_WIN64)
	DIRECTX11,
    DIRECTX12,
#endif
};

/**
 * @class WindowEnum
 * @brief List of window API.
 */
enum class WindowEnum {
    NONE,
    SDL2,
};

/**
 * @class FullScreenEnum
 * @brief List of full screen mode.
 */
enum class FullScreenEnum {
	WINDOW,
	FULLSCREEN,
    FULLSCREEN_DESKTOP,
};

/**
 * @brief Key definition for use in the input interface.
 * For now there is only the 2 shift key that are defined here, but this could increase.
 */
constexpr char KEY_LSHIFT = static_cast<char>(0xe1);
constexpr char KEY_RSHIFT = static_cast<char>(0xe5);

}  // End namespace frame.
