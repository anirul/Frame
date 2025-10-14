#include "frame/vulkan/texture.h"

#include <algorithm>
#include <cstring>

namespace frame::vulkan
{

namespace
{

std::uint8_t ComputeBytesPerPixel(
    const frame::proto::Texture& proto, std::uint8_t fallback)
{
    const auto element_bytes = Texture::BytesPerComponent(
        proto.pixel_element_size().value());
    const auto component_count = Texture::ComponentCount(
        proto.pixel_structure().value());
    if (element_bytes == 0 || component_count == 0)
    {
        return fallback;
    }
    return static_cast<std::uint8_t>(element_bytes * component_count);
}

} // namespace

Texture::Texture(
    const frame::proto::Texture& proto, glm::uvec2 display_size)
{
    auto proto_copy = proto;
    FromProto(std::move(proto_copy));
    display_size_ = display_size;
    if (proto.has_size())
    {
        size_ = glm::uvec2(
            static_cast<std::uint32_t>(proto.size().x()),
            static_cast<std::uint32_t>(proto.size().y()));
    }
    else
    {
        size_ = display_size;
    }
    bytes_per_pixel_ = ComputeBytesPerPixel(proto, bytes_per_pixel_);
    if (proto.has_pixels())
    {
        const auto& pixels = proto.pixels();
        data_.assign(
            reinterpret_cast<const std::uint8_t*>(pixels.data()),
            reinterpret_cast<const std::uint8_t*>(pixels.data()) + pixels.size());
    }
    else
    {
        const std::size_t total_size =
            static_cast<std::size_t>(size_.x) * size_.y * bytes_per_pixel_;
        data_.assign(total_size, 0);
    }
}

void Texture::Clear(const glm::vec4 color)
{
    const auto component_count =
        ComponentCount(GetData().pixel_structure().value());
    const auto bytes_per_component =
        BytesPerComponent(GetData().pixel_element_size().value());
    if (component_count == 0 || bytes_per_component == 0)
    {
        std::fill(data_.begin(), data_.end(), 0);
        SyncProtoPixels();
        return;
    }

    const std::size_t pixel_count =
        static_cast<std::size_t>(size_.x) * size_.y;
    data_.resize(pixel_count * component_count * bytes_per_component);

    for (std::size_t i = 0; i < pixel_count; ++i)
    {
        for (std::uint32_t c = 0; c < component_count; ++c)
        {
            const float channel =
                (c < 4) ? color[c] : color[3]; // repeat alpha if needed
            const float clamped = std::clamp(channel, 0.0f, 1.0f);
            auto offset = i * component_count * bytes_per_component +
                          c * bytes_per_component;
            switch (bytes_per_component)
            {
            case 1: {
                const auto value = static_cast<std::uint8_t>(clamped * 255.0f);
                data_[offset] = value;
                break;
            }
            case 2: {
                const auto value =
                    static_cast<std::uint16_t>(clamped * 65535.0f);
                std::memcpy(&data_[offset], &value, sizeof(value));
                break;
            }
            case 4: {
                const float value = clamped;
                std::memcpy(&data_[offset], &value, sizeof(value));
                break;
            }
            default:
                std::fill(
                    data_.begin() + static_cast<std::ptrdiff_t>(offset),
                    data_.begin() + static_cast<std::ptrdiff_t>(
                                       offset + bytes_per_component),
                    0);
                break;
            }
        }
    }
    SyncProtoPixels();
}

std::vector<std::uint8_t> Texture::GetTextureByte() const
{
    return data_;
}

std::vector<std::uint16_t> Texture::GetTextureWord() const
{
    const std::size_t count = data_.size() / sizeof(std::uint16_t);
    std::vector<std::uint16_t> out(count, 0);
    std::memcpy(out.data(), data_.data(), count * sizeof(std::uint16_t));
    return out;
}

std::vector<std::uint32_t> Texture::GetTextureDWord() const
{
    const std::size_t count = data_.size() / sizeof(std::uint32_t);
    std::vector<std::uint32_t> out(count, 0);
    std::memcpy(out.data(), data_.data(), count * sizeof(std::uint32_t));
    return out;
}

std::vector<float> Texture::GetTextureFloat() const
{
    const std::size_t count = data_.size() / sizeof(float);
    std::vector<float> out(count, 0.0f);
    std::memcpy(out.data(), data_.data(), count * sizeof(float));
    return out;
}

void Texture::Update(
    std::vector<std::uint8_t>&& vector,
    glm::uvec2 size,
    std::uint8_t bytes_per_pixel)
{
    data_ = std::move(vector);
    size_ = size;
    bytes_per_pixel_ = bytes_per_pixel;
    SyncProtoSize();
    SyncProtoPixels();
}

std::uint8_t Texture::BytesPerComponent(proto::PixelElementSize::Enum value)
{
    switch (value)
    {
    case proto::PixelElementSize::BYTE:
        return 1;
    case proto::PixelElementSize::SHORT:
    case proto::PixelElementSize::HALF:
        return 2;
    case proto::PixelElementSize::FLOAT:
        return 4;
    default:
        return 0;
    }
}

std::uint8_t Texture::ComponentCount(proto::PixelStructure::Enum value)
{
    switch (value)
    {
    case proto::PixelStructure::GREY:
        return 1;
    case proto::PixelStructure::GREY_ALPHA:
        return 2;
    case proto::PixelStructure::RGB:
    case proto::PixelStructure::BGR:
        return 3;
    case proto::PixelStructure::RGB_ALPHA:
    case proto::PixelStructure::BGR_ALPHA:
        return 4;
    case proto::PixelStructure::DEPTH:
        return 1;
    default:
        return 4;
    }
}

void Texture::SyncProtoSize()
{
    auto& proto = GetData();
    auto* proto_size = proto.mutable_size();
    proto_size->set_x(static_cast<int>(size_.x));
    proto_size->set_y(static_cast<int>(size_.y));
}

void Texture::SyncProtoPixels()
{
    auto& proto = GetData();
    proto.set_pixels(data_.data(), data_.size());
}

} // namespace frame::vulkan
