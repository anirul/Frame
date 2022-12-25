#pragma once

#include <filesystem>

#include "frame/device_interface.h"
#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::proto {

/**
 * @brief Parse a level as a proto represented as a path to an level.
 * @param size: Screen size.
 * @param proto_level: Protobuf to be parsed.
 * @param device: Device pointer.
 * @return A unique pointer to a level interface.
 */
std::unique_ptr<LevelInterface> ParseLevel(glm::uvec2 size, const std::filesystem::path& path,
                                           DeviceInterface* device);
/**
 * @brief Parse a level as a proto represented as a string to an level.
 * @param size: Screen size.
 * @param content: String of the JSON.
 * @param device: Device pointer.
 * @return A unique pointer to a level interface.
 */
std::unique_ptr<LevelInterface> ParseLevel(glm::uvec2 size, const std::string& content,
                                           DeviceInterface* device);

}  // End namespace frame::proto.
