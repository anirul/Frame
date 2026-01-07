#include "frame/vulkan/json/parse_texture.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <format>
#include <numbers>
#include <optional>
#include <string_view>
#include <stdexcept>
#include <unordered_map>

#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/file/image_cache.h"
#include "frame/logger.h"
#include "frame/vulkan/texture.h"

namespace frame::vulkan::json
{

namespace
{

glm::vec3 SampleEquirectangular(
    const frame::file::Image& image,
    const glm::vec2 uv)
{
    const auto size = image.GetSize();
    const auto* base = static_cast<const std::uint8_t*>(image.Data());
    const auto element = image.GetPixelElementSize().value();
    const auto structure = image.GetPixelStructure().value();
    const std::uint8_t bytes_per_component =
        frame::vulkan::Texture::BytesPerComponent(element);
    const std::uint8_t component_count =
        frame::vulkan::Texture::ComponentCount(structure);

    const float u = std::clamp(uv.x, 0.0f, 0.999999f);
    const float v = std::clamp(uv.y, 0.0f, 0.999999f);
    const std::uint32_t x = static_cast<std::uint32_t>(u * size.x);
    const std::uint32_t y = static_cast<std::uint32_t>(v * size.y);
    const std::size_t stride = component_count * bytes_per_component;
    const std::size_t offset =
        (static_cast<std::size_t>(y) * size.x + x) * stride;
    auto read_component = [&](std::uint32_t idx) -> float {
        if (idx >= component_count)
        {
            return 1.0f;
        }
        const auto* ptr = base + offset + idx * bytes_per_component;
        switch (element)
        {
        case frame::proto::PixelElementSize::BYTE:
            return static_cast<float>(*ptr) / 255.0f;
        case frame::proto::PixelElementSize::SHORT: {
            std::uint16_t val = 0;
            std::memcpy(&val, ptr, sizeof(val));
            return static_cast<float>(val) / 65535.0f;
        }
        case frame::proto::PixelElementSize::HALF: {
            std::uint16_t val = 0;
            std::memcpy(&val, ptr, sizeof(val));
            return glm::unpackHalf2x16(val).x;
        }
        case frame::proto::PixelElementSize::FLOAT: {
            float val = 0.0f;
            std::memcpy(&val, ptr, sizeof(val));
            return val;
        }
        default:
            return 0.0f;
        }
    };

    return glm::vec3(read_component(0), read_component(1), read_component(2));
}

// Convert an equirectangular image into 6 cubemap faces packed consecutively.
std::vector<std::uint8_t> ConvertEquirectangularToCubemap(
    const frame::file::Image& image,
    glm::uvec2& out_face_size,
    frame::proto::PixelElementSize element_size,
    std::uint8_t bytes_per_pixel)
{
    const auto size = image.GetSize();
    // Typical equirectangular is 2:1. Derive square face size conservatively.
    const std::uint32_t face = static_cast<std::uint32_t>(
        std::max<std::uint32_t>(1, std::min(size.x / 4, size.y / 3)));
    out_face_size = {face, face};
    const std::size_t face_pixels = static_cast<std::size_t>(face) * face;
    std::vector<std::uint8_t> out(face_pixels * 6 * bytes_per_pixel, 0);

    auto dir_from_face_uv = [](int face_idx, glm::vec2 uv) -> glm::vec3 {
        // uv in [0,1]
        const float a = 2.0f * uv.x - 1.0f;
        const float b = 2.0f * uv.y - 1.0f;
        switch (face_idx)
        {
        case 0: return glm::normalize(glm::vec3(1.0f, -b, -a));  // +X
        case 1: return glm::normalize(glm::vec3(-1.0f, -b, a));  // -X
        case 2: return glm::normalize(glm::vec3(a, 1.0f, b));    // +Y
        case 3: return glm::normalize(glm::vec3(a, -1.0f, -b));  // -Y
        case 4: return glm::normalize(glm::vec3(a, -b, 1.0f));   // +Z
        case 5: return glm::normalize(glm::vec3(-a, -b, -1.0f)); // -Z
        default: return glm::vec3(0.0f);
        }
    };

    auto write_pixel = [&](std::size_t base_offset, const glm::vec3& rgb) {
        switch (element_size.value())
        {
        case frame::proto::PixelElementSize::BYTE: {
            out[base_offset + 0] =
                static_cast<std::uint8_t>(std::clamp(rgb.r, 0.0f, 1.0f) * 255.0f);
            out[base_offset + 1] =
                static_cast<std::uint8_t>(std::clamp(rgb.g, 0.0f, 1.0f) * 255.0f);
            out[base_offset + 2] =
                static_cast<std::uint8_t>(std::clamp(rgb.b, 0.0f, 1.0f) * 255.0f);
            out[base_offset + 3] = 255;
            break;
        }
        case frame::proto::PixelElementSize::SHORT: {
            auto store = [&](std::size_t channel, float value) {
                const float clamped = std::clamp(value, 0.0f, 1.0f);
                std::uint16_t v = static_cast<std::uint16_t>(clamped * 65535.0f);
                std::memcpy(
                    out.data() + base_offset + channel * sizeof(std::uint16_t),
                    &v,
                    sizeof(v));
            };
            store(0, rgb.r);
            store(1, rgb.g);
            store(2, rgb.b);
            store(3, 1.0f);
            break;
        }
        case frame::proto::PixelElementSize::HALF: {
            auto store = [&](std::size_t channel, float value) {
                const glm::uint packed = glm::packHalf2x16(
                    glm::vec2(value, 0.0f));
                std::uint16_t v = static_cast<std::uint16_t>(packed & 0xFFFFu);
                std::memcpy(
                    out.data() + base_offset + channel * sizeof(std::uint16_t),
                    &v,
                    sizeof(v));
            };
            store(0, rgb.r);
            store(1, rgb.g);
            store(2, rgb.b);
            store(3, 1.0f);
            break;
        }
        case frame::proto::PixelElementSize::FLOAT: {
            auto store = [&](std::size_t channel, float value) {
                std::memcpy(
                    out.data() + base_offset + channel * sizeof(float),
                    &value,
                    sizeof(float));
            };
            store(0, rgb.r);
            store(1, rgb.g);
            store(2, rgb.b);
            store(3, 1.0f);
            break;
        }
        default:
            break;
        }
    };

    for (int face_idx = 0; face_idx < 6; ++face_idx)
    {
        for (std::uint32_t y = 0; y < face; ++y)
        {
            for (std::uint32_t x = 0; x < face; ++x)
            {
                const glm::vec2 uv(
                    (static_cast<float>(x) + 0.5f) / static_cast<float>(face),
                    (static_cast<float>(y) + 0.5f) / static_cast<float>(face));
                glm::vec3 dir = dir_from_face_uv(face_idx, uv);
                const float theta = std::atan2(dir.z, dir.x);
                const float phi = std::asin(std::clamp(dir.y, -1.0f, 1.0f));
                const glm::vec2 uv_eq(
                    0.5f + theta / (2.0f * std::numbers::pi_v<float>),
                    0.5f - phi / std::numbers::pi_v<float>);
                glm::vec3 sample = SampleEquirectangular(image, uv_eq);

                const std::size_t pixel_index =
                    (static_cast<std::size_t>(face_idx) * face_pixels) +
                    (static_cast<std::size_t>(y) * face + x);
                const std::size_t base = pixel_index * bytes_per_pixel;
                write_pixel(base, sample);
            }
        }
    }

    return out;
}

void ValidateTexture(const frame::proto::Texture& proto_texture)
{
    if (proto_texture.pixel_element_size().value() ==
        frame::proto::PixelElementSize::INVALID)
    {
        throw std::runtime_error("Invalid pixel element size for texture.");
    }
    if (proto_texture.pixel_structure().value() ==
        frame::proto::PixelStructure::INVALID)
    {
        throw std::runtime_error("Invalid pixel structure for texture.");
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

bool CachedCubemapLooksClamped(
    const frame::file::ImageCachePayload& payload,
    frame::proto::PixelElementSize::Enum expected_element)
{
    if (payload.data.empty())
    {
        return false;
    }
    if (payload.element_size != expected_element)
    {
        return false;
    }
    if (expected_element != frame::proto::PixelElementSize::HALF &&
        expected_element != frame::proto::PixelElementSize::FLOAT)
    {
        return false;
    }
    constexpr float kEpsilon = 1e-3f;
    const auto& bytes = payload.data;
    if (expected_element == frame::proto::PixelElementSize::FLOAT)
    {
        const std::size_t count = bytes.size() / sizeof(float);
        for (std::size_t i = 0; i < count; ++i)
        {
            float value = 0.0f;
            std::memcpy(
                &value,
                bytes.data() + i * sizeof(float),
                sizeof(float));
            if (value > 1.0f + kEpsilon)
            {
                return false;
            }
        }
        return true;
    }
    const std::size_t count = bytes.size() / sizeof(std::uint16_t);
    for (std::size_t i = 0; i < count; ++i)
    {
        std::uint16_t packed = 0;
        std::memcpy(
            &packed,
            bytes.data() + i * sizeof(std::uint16_t),
            sizeof(std::uint16_t));
        const float value = glm::unpackHalf2x16(packed).x;
        if (value > 1.0f + kEpsilon)
        {
            return false;
        }
    }
    return true;
}

std::string BuildCubemapCacheKey(
    const std::filesystem::path& source_path,
    frame::proto::PixelElementSize::Enum element_size,
    frame::proto::PixelStructure::Enum structure,
    std::uint32_t desired_channels)
{
    std::string structure_label = SanitizeLabel(
        frame::proto::PixelStructure_Enum_Name(structure));
    std::string element_label = SanitizeLabel(
        frame::proto::PixelElementSize_Enum_Name(element_size));
    return std::format(
        "{}|{}|{}|{}|{}ch",
        frame::file::PurifyFilePath(source_path),
        "cubemap",
        structure_label,
        element_label,
        desired_channels);
}

std::optional<frame::file::ImageCacheMetadata> BuildCubemapCacheMetadata(
    const std::filesystem::path& source_path,
    frame::proto::PixelElementSize::Enum element_size,
    frame::proto::PixelStructure::Enum structure,
    std::uint32_t desired_channels)
{
    std::error_code metadata_error;
    const auto source_size =
        std::filesystem::file_size(source_path, metadata_error);
    if (metadata_error)
    {
        return std::nullopt;
    }
    auto write_time =
        std::filesystem::last_write_time(source_path, metadata_error);
    if (metadata_error)
    {
        return std::nullopt;
    }
    const auto mtime_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            write_time.time_since_epoch())
            .count();

    frame::file::ImageCacheMetadata metadata;
    metadata.source_relative = frame::file::PurifyFilePath(source_path);
    metadata.source_size = static_cast<std::uint64_t>(source_size);
    metadata.source_mtime_ns = static_cast<std::uint64_t>(mtime_ns);

    try
    {
        const auto asset_root = frame::file::FindDirectory("asset");
        auto cache_root = (asset_root / "cache").lexically_normal();
        std::error_code relative_error;
        auto relative =
            std::filesystem::relative(source_path, asset_root, relative_error);
        if (relative_error)
        {
            relative = source_path.filename();
        }
        auto cache_dir = cache_root;
        if (relative.has_parent_path() && relative.parent_path() != ".")
        {
            cache_dir /= relative.parent_path();
        }
        std::string stem = relative.stem().string();
        if (stem.empty())
        {
            stem = source_path.stem().string();
        }
        std::string structure_label = SanitizeLabel(
            frame::proto::PixelStructure_Enum_Name(structure));
        std::string element_label = SanitizeLabel(
            frame::proto::PixelElementSize_Enum_Name(element_size));
        std::string cache_filename = std::format(
            "{}-{}-{}-{}-{}ch.imgpb",
            stem,
            "cubemap",
            structure_label,
            element_label,
            desired_channels);
        metadata.cache_path = (cache_dir / cache_filename).lexically_normal();
        metadata.cache_relative =
            frame::file::PurifyFilePath(metadata.cache_path);
        return metadata;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

std::filesystem::path BuildCubemapCachePath(
    const std::filesystem::path& source_path,
    frame::proto::PixelElementSize::Enum element_size,
    frame::proto::PixelStructure::Enum structure,
    std::uint32_t desired_channels)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    auto cache_root = (asset_root / "cache").lexically_normal();
    std::error_code relative_error;
    auto relative =
        std::filesystem::relative(source_path, asset_root, relative_error);
    if (relative_error)
    {
        relative = source_path.filename();
    }
    auto cache_dir = cache_root;
    if (relative.has_parent_path() && relative.parent_path() != ".")
    {
        cache_dir /= relative.parent_path();
    }
    std::string stem = relative.stem().string();
    if (stem.empty())
    {
        stem = source_path.stem().string();
    }
    std::string structure_label = SanitizeLabel(
        frame::proto::PixelStructure_Enum_Name(structure));
    std::string element_label = SanitizeLabel(
        frame::proto::PixelElementSize_Enum_Name(element_size));
    std::string cache_filename = std::format(
        "{}-{}-{}-{}-{}ch.imgpb",
        stem,
        "cubemap",
        structure_label,
        element_label,
        desired_channels);
    return (cache_dir / cache_filename).lexically_normal();
}

std::unordered_map<std::string, frame::file::ImageCachePayload> g_cubemap_cache;

} // namespace

std::unique_ptr<frame::TextureInterface> ParseTexture(
    const frame::proto::Texture& proto_texture, glm::uvec2 size)
{
    ValidateTexture(proto_texture);
    auto texture = std::make_unique<frame::vulkan::Texture>(
        proto_texture, size);

    if (proto_texture.cubemap())
    {
        texture->SetViewType(vk::ImageViewType::eCube);
    }

    if (proto_texture.has_file_name())
    {
        frame::Logger& logger = frame::Logger::GetInstance();
        const auto bytes_per_component =
            frame::vulkan::Texture::BytesPerComponent(
                proto_texture.pixel_element_size().value());
        auto component_count = frame::vulkan::Texture::ComponentCount(
            proto_texture.pixel_structure().value());
        std::optional<frame::file::ImageCacheMetadata> cubemap_cache_metadata;
        std::optional<frame::file::ImageCachePayload> cached_cubemap;
        std::string cubemap_cache_key;
        const std::uint32_t cubemap_channels = 4;
        const auto cubemap_structure =
            frame::proto::PixelStructure::RGB_ALPHA;
        std::optional<std::filesystem::path> cubemap_cache_path;
        bool cubemap_cache_relaxed = false;

        if (proto_texture.cubemap())
        {
            bool source_available = true;
            std::filesystem::path absolute_path;
            try
            {
                const auto resolved_path =
                    frame::file::FindFile(proto_texture.file_name());
                absolute_path =
                    std::filesystem::absolute(resolved_path).lexically_normal();
            }
            catch (const std::exception&)
            {
                source_available = false;
                const auto asset_root = frame::file::FindDirectory("asset");
                auto relative = StripAssetPrefix(proto_texture.file_name());
                absolute_path = (asset_root / relative).lexically_normal();
            }
            cubemap_cache_key = BuildCubemapCacheKey(
                absolute_path,
                proto_texture.pixel_element_size().value(),
                cubemap_structure,
                cubemap_channels);
            auto in_memory = g_cubemap_cache.find(cubemap_cache_key);
            if (in_memory != g_cubemap_cache.end())
            {
                cached_cubemap = in_memory->second;
            }
            else
            {
                cubemap_cache_metadata = BuildCubemapCacheMetadata(
                    absolute_path,
                    proto_texture.pixel_element_size().value(),
                    cubemap_structure,
                    cubemap_channels);
                if (cubemap_cache_metadata)
                {
                    cached_cubemap = frame::file::LoadImageCache(
                        *cubemap_cache_metadata,
                        proto_texture.pixel_element_size().value(),
                        cubemap_structure,
                        cubemap_channels);
                    if (cached_cubemap)
                    {
                        cubemap_cache_path = cubemap_cache_metadata->cache_path;
                    }
                }
                else if (!source_available)
                {
                    const auto cache_path = BuildCubemapCachePath(
                        absolute_path,
                        proto_texture.pixel_element_size().value(),
                        cubemap_structure,
                        cubemap_channels);
                    cached_cubemap = frame::file::LoadImageCacheRelaxed(
                        cache_path,
                        proto_texture.pixel_element_size().value(),
                        cubemap_structure,
                        cubemap_channels);
                    if (cached_cubemap)
                    {
                        cubemap_cache_path = cache_path;
                        cubemap_cache_relaxed = true;
                    }
                }
            }

            const auto element_size = proto_texture.pixel_element_size().value();
            const bool has_source = source_available;
            if (cached_cubemap && has_source &&
                CachedCubemapLooksClamped(*cached_cubemap, element_size))
            {
                const std::string cache_label = cubemap_cache_path
                    ? frame::file::PurifyFilePath(*cubemap_cache_path)
                    : cubemap_cache_key;
                logger->warn(
                    "Cubemap cache {} appears clamped; regenerating from source.",
                    cache_label);
                cached_cubemap.reset();
                g_cubemap_cache.erase(cubemap_cache_key);
            }
            else if (cached_cubemap && !cached_cubemap->data.empty())
            {
                if (cubemap_cache_path)
                {
                    if (cubemap_cache_relaxed)
                    {
                        logger->info(
                            "Loaded cubemap cache {} (source missing).",
                            frame::file::PurifyFilePath(*cubemap_cache_path));
                    }
                    else
                    {
                        logger->info(
                            "Loaded cubemap cache {}.",
                            frame::file::PurifyFilePath(*cubemap_cache_path));
                    }
                }
                g_cubemap_cache.emplace(
                    cubemap_cache_key, *cached_cubemap);
            }
        }

        if (proto_texture.cubemap() && cached_cubemap &&
            !cached_cubemap->data.empty())
        {
            component_count = cubemap_channels;
            texture->GetData().mutable_pixel_structure()->set_value(
                cubemap_structure);
            auto bytes_per_pixel =
                static_cast<std::uint8_t>(bytes_per_component * component_count);
            glm::uvec2 face_size = {
                static_cast<std::uint32_t>(cached_cubemap->size.x),
                static_cast<std::uint32_t>(cached_cubemap->size.y)};
            texture->Update(
                std::vector<std::uint8_t>(cached_cubemap->data),
                face_size,
                bytes_per_pixel);
            texture->SetSerializeEnable(true);
            return texture;
        }

        frame::file::Image image(
            proto_texture.file_name(),
            proto_texture.pixel_element_size(),
            proto_texture.pixel_structure());
        const auto image_size = image.GetSize();
        if (proto_texture.cubemap())
        {
            component_count = cubemap_channels; // force RGBA faces
            texture->GetData().mutable_pixel_structure()->set_value(
                cubemap_structure);
        }
        auto bytes_per_pixel =
            static_cast<std::uint8_t>(bytes_per_component * component_count);

        if (proto_texture.cubemap())
        {
            glm::uvec2 face_size{};
            auto pixels = ConvertEquirectangularToCubemap(
                image,
                face_size,
                proto_texture.pixel_element_size(),
                bytes_per_pixel);
            if (cubemap_cache_metadata)
            {
                frame::file::SaveImageCache(
                    *cubemap_cache_metadata,
                    proto_texture.pixel_element_size().value(),
                    cubemap_structure,
                    cubemap_channels,
                    glm::ivec2(
                        static_cast<int>(face_size.x),
                        static_cast<int>(face_size.y)),
                    pixels);
                logger->info(
                    "Saved cubemap cache {}.",
                    cubemap_cache_metadata->cache_relative);
            }
            if (!cubemap_cache_key.empty())
            {
                frame::file::ImageCachePayload payload;
                payload.size = glm::ivec2(
                    static_cast<int>(face_size.x),
                    static_cast<int>(face_size.y));
                payload.element_size = proto_texture.pixel_element_size().value();
                payload.structure = cubemap_structure;
                payload.desired_channels = cubemap_channels;
                payload.data = pixels;
                g_cubemap_cache.emplace(
                    cubemap_cache_key, std::move(payload));
            }
            texture->Update(std::move(pixels), face_size, bytes_per_pixel);
        }
        else
        {
            const std::size_t total_bytes =
                static_cast<std::size_t>(image_size.x) * image_size.y *
                bytes_per_pixel;
            std::vector<std::uint8_t> pixels(total_bytes, 0);
            const auto* src = static_cast<const std::uint8_t*>(image.Data());
            if (src && total_bytes > 0)
            {
                std::memcpy(pixels.data(), src, total_bytes);
                texture->Update(std::move(pixels), image_size, bytes_per_pixel);
            }
        }
    }
    texture->SetSerializeEnable(true);
    return texture;
}

} // namespace frame::vulkan::json
