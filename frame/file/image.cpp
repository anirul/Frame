#include "frame/file/image.h"

// Removed the warning from sprintf.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable : 4996)
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <set>
#include <string_view>
#include <vector>
// This will be included if needed in the "image_stb.h".
// #define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "frame/file/file_system.h"
#include "frame/file/image_cache.h"
#include "frame/logger.h"

namespace frame::file
{

namespace
{

int DesiredChannels(const proto::PixelStructure& pixel_structure)
{
    switch (pixel_structure.value())
    {
    case proto::PixelStructure::GREY:
    case proto::PixelStructure::DEPTH:
        return 1;
    case proto::PixelStructure::GREY_ALPHA:
        return 2;
    case proto::PixelStructure::RGB:
    case proto::PixelStructure::BGR:
        return 3;
    case proto::PixelStructure::RGB_ALPHA:
    case proto::PixelStructure::BGR_ALPHA:
        return 4;
    default:
        throw std::runtime_error(
            "unsupported pixel structure : " +
            std::to_string(static_cast<int>(pixel_structure.value())));
    }
}

std::size_t BytesPerElement(proto::PixelElementSize::Enum element_size)
{
    switch (element_size)
    {
    case proto::PixelElementSize::BYTE:
        return 1;
    case proto::PixelElementSize::SHORT:
        return 2;
    case proto::PixelElementSize::HALF:
        // stb_image returns 32-bit floats even when requesting half floats.
        // Preserve the original behaviour (feeding floats to OpenGL while
        // advertising half precision) so we cache the same 4-byte stride.
        return 4;
    case proto::PixelElementSize::FLOAT:
        return 4;
    default:
        throw std::runtime_error(
            "unsupported element size : " +
            std::to_string(static_cast<int>(element_size)));
    }
}

std::string SanitizeLabel(std::string label)
{
    for (auto& ch : label)
    {
        if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_')
        {
            ch = '_';
        }
    }
    return label;
}

std::filesystem::path StripAssetPrefix(std::filesystem::path input)
{
    if (input.is_absolute())
    {
        return input;
    }
    const std::string generic = input.generic_string();
    constexpr std::string_view kPrefix = "asset/";
    if (generic.rfind(kPrefix, 0) == 0)
    {
        return std::filesystem::path(generic.substr(kPrefix.size()));
    }
    return input;
}

} // namespace

Image::Image(
    const std::filesystem::path& file,
    proto::PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/,
    proto::PixelStructure pixel_structure /*= PixelStructure::RGB*/)
    : pixel_element_size_(pixel_element_size), pixel_structure_(pixel_structure)
{
    const auto& logger = frame::Logger::GetInstance();
    bool source_available = true;
    std::filesystem::path absolute_path;
    try
    {
        const auto resolved_path = frame::file::FindFile(file);
        absolute_path =
            std::filesystem::absolute(resolved_path).lexically_normal();
    }
    catch (const std::exception&)
    {
        source_available = false;
        const auto asset_root = frame::file::FindDirectory("asset");
        auto relative = StripAssetPrefix(file);
        absolute_path = (asset_root / relative).lexically_normal();
    }
    const int desired_channels = DesiredChannels(pixel_structure);

    std::optional<ImageCacheMetadata> cache_metadata;
    std::optional<std::filesystem::file_time_type> source_write_time;
    std::filesystem::path cache_path;
    std::string cache_relative;
    bool cache_path_ready = false;
    try
    {
        const auto asset_root = frame::file::FindDirectory("asset");
        auto cache_root = (asset_root / "cache").lexically_normal();
        std::error_code relative_error;
        auto relative = std::filesystem::relative(
            absolute_path, asset_root, relative_error);
        if (relative_error)
        {
            relative = absolute_path.filename();
        }
        auto cache_dir = cache_root;
        if (relative.has_parent_path() && relative.parent_path() != ".")
        {
            cache_dir /= relative.parent_path();
        }
        std::string stem = relative.stem().string();
        if (stem.empty())
        {
            stem = absolute_path.stem().string();
        }
        std::string structure_label = SanitizeLabel(
            proto::PixelStructure_Enum_Name(pixel_structure.value()));
        std::string element_label = SanitizeLabel(
            proto::PixelElementSize_Enum_Name(
                pixel_element_size.value()));
        std::string cache_filename = std::format(
            "{}-{}-{}-{}ch.imgpb",
            stem,
            structure_label,
            element_label,
            desired_channels);
        cache_path = (cache_dir / cache_filename).lexically_normal();
        cache_relative = frame::file::PurifyFilePath(cache_path);
        cache_path_ready = true;
    }
    catch (const std::exception& exception)
    {
        logger->warn("Image cache disabled: {}", exception.what());
    }

    if (source_available)
    {
        std::error_code metadata_error;
        const auto source_size =
            std::filesystem::file_size(absolute_path, metadata_error);
        if (!metadata_error)
        {
            auto write_time =
                std::filesystem::last_write_time(absolute_path, metadata_error);
            if (!metadata_error)
            {
                source_write_time = write_time;
            }
            if (!metadata_error && cache_path_ready)
            {
                const auto mtime_ns =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        write_time.time_since_epoch())
                        .count();
                ImageCacheMetadata metadata;
                metadata.source_relative =
                    frame::file::PurifyFilePath(absolute_path);
                metadata.source_size = static_cast<std::uint64_t>(source_size);
                metadata.source_mtime_ns = static_cast<std::uint64_t>(mtime_ns);
                metadata.cache_path = cache_path;
                metadata.cache_relative = cache_relative;
                cache_metadata = std::move(metadata);
            }
        }
        else
        {
            logger->info(
                "Image cache disabled for {}: unable to inspect source file.",
                absolute_path.string());
        }
    }

    if (cache_metadata)
    {
        auto cached = LoadImageCache(
            *cache_metadata,
            static_cast<proto::PixelElementSize::Enum>(
                pixel_element_size.value()),
            static_cast<proto::PixelStructure::Enum>(pixel_structure.value()),
            static_cast<std::uint32_t>(desired_channels));
        if (cached)
        {
            if (!cached->data.empty())
            {
                void* data = std::malloc(cached->data.size());
                if (data)
                {
                    std::memcpy(data, cached->data.data(), cached->data.size());
                    image_ = data;
                    free_ = true;
                    size_ = cached->size;
                    logger->info(
                        "Loaded image cache {}.",
                        cache_metadata->cache_relative);
                    return;
                }
                logger->warn(
                    "Failed to allocate memory for cached image {}.",
                    cache_metadata->cache_relative);
            }
        }
    }
    if (source_available && cache_path_ready && source_write_time)
    {
        std::error_code cache_time_error;
        const auto cache_write_time =
            std::filesystem::last_write_time(cache_path, cache_time_error);
        if (!cache_time_error && cache_write_time >= *source_write_time)
        {
            auto cached = LoadImageCacheRelaxed(
                cache_path,
                static_cast<proto::PixelElementSize::Enum>(
                    pixel_element_size.value()),
                static_cast<proto::PixelStructure::Enum>(
                    pixel_structure.value()),
                static_cast<std::uint32_t>(desired_channels));
            if (cached)
            {
                if (!cached->data.empty())
                {
                    void* data = std::malloc(cached->data.size());
                    if (data)
                    {
                        std::memcpy(
                            data,
                            cached->data.data(),
                            cached->data.size());
                        image_ = data;
                        free_ = true;
                        size_ = cached->size;
                        logger->info(
                            "Loaded image cache {} (source present).",
                            cache_relative);
                        return;
                    }
                    logger->warn(
                        "Failed to allocate memory for cached image {}.",
                        cache_relative);
                }
            }
        }
    }
    else if (!source_available && cache_path_ready)
    {
        auto cached = LoadImageCacheRelaxed(
            cache_path,
            static_cast<proto::PixelElementSize::Enum>(
                pixel_element_size.value()),
            static_cast<proto::PixelStructure::Enum>(pixel_structure.value()),
            static_cast<std::uint32_t>(desired_channels));
        if (cached)
        {
            if (!cached->data.empty())
            {
                void* data = std::malloc(cached->data.size());
                if (data)
                {
                    std::memcpy(data, cached->data.data(), cached->data.size());
                    image_ = data;
                    free_ = true;
                    size_ = cached->size;
                    logger->info(
                        "Loaded image cache {} (source missing).",
                        cache_relative);
                    return;
                }
                logger->warn(
                    "Failed to allocate memory for cached image {}.",
                    cache_relative);
            }
        }
    }

    if (!source_available)
    {
        throw std::runtime_error(std::format(
            "Image source [{}] not found and cache missing.",
            absolute_path.string()));
    }

    logger->info("Openning image: [{}].", absolute_path.string());
    int channels;
    // This is in the case of OpenGL (for now the only case).
    stbi_set_flip_vertically_on_load(true);
    glm::ivec2 size = glm::ivec2(0, 0);
    switch (pixel_element_size.value())
    {
    case proto::PixelElementSize::BYTE: {
        image_ = stbi_load(
            absolute_path.string().c_str(),
            &size.x,
            &size.y,
            &channels,
            desired_channels);
        break;
    }
    case proto::PixelElementSize::SHORT: {
        image_ = stbi_load_16(
            absolute_path.string().c_str(),
            &size.x,
            &size.y,
            &channels,
            desired_channels);
        break;
    }
    case proto::PixelElementSize::HALF:
        [[fallthrough]];
    case proto::PixelElementSize::FLOAT: {
        image_ = stbi_loadf(
            absolute_path.string().c_str(),
            &size.x,
            &size.y,
            &channels,
            desired_channels);
        break;
    }
    default:
        throw std::runtime_error(
            "unsupported element size : " +
            std::to_string(static_cast<int>(pixel_element_size_.value())));
    }
    if (!image_)
    {
        std::string stbi_error = stbi_failure_reason();
        throw std::runtime_error(
            std::format(
                "unsupported file: [{}], reason: {}",
                absolute_path.string(),
                stbi_error));
    }
    free_ = true;
    size_ = size;

    if (cache_metadata)
    {
        const std::size_t bytes_per_element = BytesPerElement(
            static_cast<proto::PixelElementSize::Enum>(
                pixel_element_size.value()));
        const std::size_t total_bytes =
            static_cast<std::size_t>(size_.x) *
            static_cast<std::size_t>(size_.y) *
            static_cast<std::size_t>(desired_channels) * bytes_per_element;
        std::vector<std::uint8_t> buffer;
        buffer.resize(total_bytes);
        std::memcpy(buffer.data(), image_, total_bytes);
        SaveImageCache(
            *cache_metadata,
            static_cast<proto::PixelElementSize::Enum>(
                pixel_element_size.value()),
            static_cast<proto::PixelStructure::Enum>(pixel_structure.value()),
            static_cast<std::uint32_t>(desired_channels),
            size_,
            buffer);
        logger->info("Saved image cache {}.", cache_metadata->cache_relative);
    }
}

Image::Image(
        glm::uvec2 size,
        proto::PixelElementSize pixel_element_size/* =
            proto::PixelElementSize_BYTE()*/,
        proto::PixelStructure pixel_structure/* =
            proto::PixelStructure_RGB()*/)
        :
        size_(size),
        pixel_element_size_(pixel_element_size),
        pixel_structure_(pixel_structure)
{
    free_ = false;
}

void Image::SaveImageToFile(const std::string& file) const
{
    // For OpenGL it seams...
    stbi_flip_vertically_on_write(true);
    const auto& logger = frame::Logger::GetInstance();
    logger->info("Saving [{}]...", file);
    if (!image_)
        throw std::runtime_error("no pointer to be saved?");
    stbi_write_png(
        file.c_str(),
        size_.x,
        size_.y,
        pixel_structure_.value(),
        image_,
        size_.x * pixel_structure_.value());
}

void Image::SetData(void* data)
{
    if (free_)
        stbi_image_free(image_);
    image_ = data;
    free_ = false;
}

Image::~Image()
{
    if (free_)
        stbi_image_free(image_);
}

} // End namespace frame::file.
