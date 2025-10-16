#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include "frame/proto/pixel.pb.h"

namespace frame::file
{

struct ImageCacheMetadata
{
    std::filesystem::path cache_path;
    std::string cache_relative;
    std::string source_relative;
    std::uint64_t source_size = 0;
    std::uint64_t source_mtime_ns = 0;
};

struct ImageCachePayload
{
    glm::ivec2 size = glm::ivec2(0);
    frame::proto::PixelElementSize::Enum element_size =
        frame::proto::PixelElementSize::INVALID;
    frame::proto::PixelStructure::Enum structure =
        frame::proto::PixelStructure::INVALID;
    std::uint32_t desired_channels = 0;
    std::vector<std::uint8_t> data;
};

std::optional<ImageCachePayload> LoadImageCache(
    const ImageCacheMetadata& metadata,
    frame::proto::PixelElementSize::Enum expected_element_size,
    frame::proto::PixelStructure::Enum expected_structure,
    std::uint32_t expected_channels);

void SaveImageCache(
    const ImageCacheMetadata& metadata,
    frame::proto::PixelElementSize::Enum element_size,
    frame::proto::PixelStructure::Enum structure,
    std::uint32_t desired_channels,
    glm::ivec2 size,
    const std::vector<std::uint8_t>& data);

} // namespace frame::file
