#pragma once

#include <filesystem>
#include <string>

#include <glm/glm.hpp>

#include "frame/json/level_data.h"
#include "frame/json/parse_level.h"
#include "frame/json/proto.h"

namespace frame::vulkan::json
{

using LevelData = frame::json::LevelData;

inline LevelData ParseLevelData(
    glm::uvec2 size,
    const frame::proto::Level& proto_level,
    const std::filesystem::path& asset_root)
{
    return frame::json::ParseLevelData(size, proto_level, asset_root);
}

inline LevelData ParseLevelData(
    glm::uvec2 size,
    const std::string& content,
    const std::filesystem::path& asset_root)
{
    return frame::json::ParseLevelData(size, content, asset_root);
}

inline LevelData ParseLevelData(
    glm::uvec2 size,
    const std::filesystem::path& path,
    const std::filesystem::path& asset_root)
{
    return frame::json::ParseLevelData(size, path, asset_root);
}

} // namespace frame::vulkan::json
