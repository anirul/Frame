#pragma once

#include <filesystem>

#include "frame/device_interface.h"
#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::proto {

/**
 * @brief Parse a level as a proto represented as a path to an level.
 * @param size: Screen size.
 * @param proto_level: Protocol buffer to be parsed.
 * @return A unique pointer to a level interface.
 */
std::unique_ptr<LevelInterface> ParseLevel(glm::uvec2 size, const std::filesystem::path& path);
/**
 * @brief Parse a level as a proto represented as a string to an level.
 * @param size: Screen size.
 * @param content: String of the JSON.
 * @return A unique pointer to a level interface.
 */
std::unique_ptr<LevelInterface> ParseLevel(glm::uvec2 size, const std::string& content);
/**
 * @brief Parse a level as a proto.
 * @param size: Screen size.
 * @param proto: Parsed protocol buffer.
 */
std::unique_ptr<LevelInterface> ParseLevel(glm::uvec2 size, const proto::Level& proto);

}  // End namespace frame::proto.
