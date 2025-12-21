#include "frame/vulkan/json/parse_texture.h"

#include <algorithm>
#include <cstring>
#include <numbers>
#include <stdexcept>

#include "frame/file/image.h"
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
        case frame::proto::PixelElementSize::SHORT:
        case frame::proto::PixelElementSize::HALF:
        case frame::proto::PixelElementSize::FLOAT: {
            auto store = [&](std::size_t channel, float value) {
                const float clamped = std::clamp(value, 0.0f, 1.0f);
                switch (element_size.value())
                {
                case frame::proto::PixelElementSize::SHORT: {
                    std::uint16_t v = static_cast<std::uint16_t>(clamped * 65535.0f);
                    std::memcpy(
                        out.data() + base_offset + channel * sizeof(std::uint16_t),
                        &v,
                        sizeof(v));
                    break;
                }
                case frame::proto::PixelElementSize::HALF: {
                    const glm::uint packed = glm::packHalf2x16(
                        glm::vec2(clamped, 0.0f));
                    std::uint16_t v = static_cast<std::uint16_t>(packed & 0xFFFFu);
                    std::memcpy(
                        out.data() + base_offset + channel * sizeof(std::uint16_t),
                        &v,
                        sizeof(v));
                    break;
                }
                case frame::proto::PixelElementSize::FLOAT: {
                    std::memcpy(
                        out.data() + base_offset + channel * sizeof(float),
                        &clamped,
                        sizeof(float));
                    break;
                }
                default:
                    break;
                }
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
        frame::file::Image image(
            proto_texture.file_name(),
            proto_texture.pixel_element_size(),
            proto_texture.pixel_structure());
        const auto image_size = image.GetSize();
        const auto bytes_per_component =
            frame::vulkan::Texture::BytesPerComponent(
                proto_texture.pixel_element_size().value());
        auto component_count =
            frame::vulkan::Texture::ComponentCount(
                proto_texture.pixel_structure().value());
        if (proto_texture.cubemap())
        {
            component_count = 4; // force RGBA faces
            texture->GetData().mutable_pixel_structure()->set_value(
                frame::proto::PixelStructure::RGB_ALPHA);
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
