#pragma once

#include <chrono>
#include <vector>
#include <string>

#include "frame/name_interface.h"

namespace frame {

/**
 * @class Stream Interface.
 * @brief Convert a stream into an interface to something usable by a Renderer.
 */
template <typename T>
class StreamInterface : public NameInterface {
   public:
    /**
     * @brief Extract a pointer to the last frame (give the pointer) in case none then give nullptr.
     * @return A pair of time and pointer to the last frame or nullptr.
     */
    virtual std::vector<T> ExtractVector() = 0;
    /**
     * @brief Get the size of the buffer (in case this is a texture it will return the width,
     * height).
     * @return The size of the texture (weight, height) or buffer (size, 1).
     */
    virtual std::pair<std::uint32_t, std::uint32_t> GetSize() const = 0;
    /**
     * @brief Get the number of bytes per pixel.
     * @return The number of byte per pixel.
     */
    virtual std::uint8_t GetBytesPerPixel() const = 0;
    /**
     * @brief Get the time of the arrival of the last frame.
     * @return A timing event that give the time the last event was copied.
     */
    virtual std::chrono::time_point<std::chrono::system_clock> GetTime() const = 0;
};

}  // End namespace frame.
