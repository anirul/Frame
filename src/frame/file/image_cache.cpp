#include "frame/file/image_cache.h"

#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>

#include "frame/logger.h"
#include "frame/proto/image_cache.pb.h"

namespace frame::file
{
namespace
{
constexpr std::uint32_t kCacheVersion = 1;

frame::proto::PixelElementSize ToPixelElementSize(
    frame::proto::PixelElementSize::Enum value)
{
    frame::proto::PixelElementSize element_size;
    element_size.set_value(value);
    return element_size;
}

frame::proto::PixelStructure ToPixelStructure(
    frame::proto::PixelStructure::Enum value)
{
    frame::proto::PixelStructure structure;
    structure.set_value(value);
    return structure;
}

Logger& GetLogger()
{
    return Logger::GetInstance();
}

} // namespace

std::optional<ImageCachePayload> LoadImageCache(
    const ImageCacheMetadata& metadata,
    frame::proto::PixelElementSize::Enum expected_element_size,
    frame::proto::PixelStructure::Enum expected_structure,
    std::uint32_t expected_channels)
{
    if (metadata.cache_path.empty())
    {
        return std::nullopt;
    }
    if (!std::filesystem::exists(metadata.cache_path))
    {
        return std::nullopt;
    }
    std::ifstream input(metadata.cache_path, std::ios::binary);
    if (!input)
    {
        GetLogger()->warn(
            "Failed to open image cache file {} for reading.",
            metadata.cache_path.string());
        return std::nullopt;
    }
    proto::ImageCache cache_proto;
    if (!cache_proto.ParseFromIstream(&input))
    {
        GetLogger()->warn(
            "Could not parse image cache {}.", metadata.cache_path.string());
        return std::nullopt;
    }
    if (cache_proto.cache_version() != kCacheVersion)
    {
        GetLogger()->info(
            "Ignoring image cache {} due to version mismatch ({} != {}).",
            metadata.cache_path.string(),
            cache_proto.cache_version(),
            kCacheVersion);
        return std::nullopt;
    }
    if (cache_proto.source_size() != metadata.source_size ||
        cache_proto.source_mtime_ns() != metadata.source_mtime_ns ||
        cache_proto.source_relative() != metadata.source_relative ||
        cache_proto.cache_relative() != metadata.cache_relative)
    {
        GetLogger()->info(
            "Ignoring image cache {} due to stale source metadata.",
            metadata.cache_path.string());
        return std::nullopt;
    }
    if (cache_proto.pixel_element_size().value() != expected_element_size ||
        cache_proto.pixel_structure().value() != expected_structure ||
        cache_proto.desired_channels() != expected_channels)
    {
        GetLogger()->info(
            "Ignoring image cache {} due to mismatched pixel format.",
            metadata.cache_path.string());
        return std::nullopt;
    }

    ImageCachePayload payload;
    payload.size = glm::ivec2(
        static_cast<int>(cache_proto.width()),
        static_cast<int>(cache_proto.height()));
    payload.element_size = cache_proto.pixel_element_size().value();
    payload.structure = cache_proto.pixel_structure().value();
    payload.desired_channels = cache_proto.desired_channels();
    payload.data.resize(cache_proto.data().size());
    std::memcpy(
        payload.data.data(),
        cache_proto.data().data(),
        cache_proto.data().size());
    return payload;
}

void SaveImageCache(
    const ImageCacheMetadata& metadata,
    frame::proto::PixelElementSize::Enum element_size,
    frame::proto::PixelStructure::Enum structure,
    std::uint32_t desired_channels,
    glm::ivec2 size,
    const std::vector<std::uint8_t>& data)
{
    if (metadata.cache_path.empty())
    {
        return;
    }
    std::error_code ec;
    const auto parent = metadata.cache_path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent, ec);
        if (ec)
        {
            GetLogger()->warn(
                "Failed to create image cache directory {}: {}",
                parent.string(),
                ec.message());
            return;
        }
    }

    proto::ImageCache cache_proto;
    cache_proto.set_cache_version(kCacheVersion);
    cache_proto.set_cache_relative(metadata.cache_relative);
    cache_proto.set_source_relative(metadata.source_relative);
    cache_proto.set_source_size(metadata.source_size);
    cache_proto.set_source_mtime_ns(metadata.source_mtime_ns);
    cache_proto.set_width(static_cast<std::uint32_t>(size.x));
    cache_proto.set_height(static_cast<std::uint32_t>(size.y));
    *cache_proto.mutable_pixel_element_size() =
        ToPixelElementSize(element_size);
    *cache_proto.mutable_pixel_structure() = ToPixelStructure(structure);
    cache_proto.set_desired_channels(desired_channels);
    cache_proto.set_data(data.data(), static_cast<int>(data.size()));

    std::ofstream output(
        metadata.cache_path, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        GetLogger()->warn(
            "Failed to open image cache file {} for writing.",
            metadata.cache_path.string());
        return;
    }
    if (!cache_proto.SerializeToOstream(&output))
    {
        GetLogger()->warn(
            "Failed to serialize image cache {}.",
            metadata.cache_path.string());
    }
}

} // namespace frame::file
