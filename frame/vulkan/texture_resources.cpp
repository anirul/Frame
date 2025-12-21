#include "frame/vulkan/texture_resources.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <glm/gtc/packing.hpp>

#include "frame/level.h"
#include "frame/vulkan/command_queue.h"
#include "frame/vulkan/device.h"
#include "frame/vulkan/gpu_memory_manager.h"

namespace frame::vulkan
{

TextureResources::TextureResources(Device& owner)
    : owner_(owner)
{
}

void TextureResources::Build(
    LevelInterface& level,
    const frame::json::LevelData& level_data)
{
    Destroy();

    for (const auto& proto_texture : level_data.proto.textures())
    {
        auto texture_id = level.GetIdFromName(proto_texture.name());
        if (!texture_id)
        {
            continue;
        }

        auto* texture_ptr = dynamic_cast<frame::vulkan::Texture*>(
            &level.GetTextureFromId(texture_id));
        if (!texture_ptr)
        {
            owner_.logger_->warn(
                "Texture {} is not a Vulkan texture instance.",
                proto_texture.name());
            continue;
        }

        try
        {
            UploadTexture(texture_id, *texture_ptr);
            textures_[texture_id] = texture_ptr;
        }
        catch (const std::exception& ex)
        {
            owner_.logger_->warn(
                "Skipping texture {}: {}",
                proto_texture.name(),
                ex.what());
            texture_ptr->ResetGpuResources();
        }
    }
}

void TextureResources::Destroy()
{
    for (auto& [id, texture_ptr] : textures_)
    {
        if (texture_ptr)
        {
            texture_ptr->ResetGpuResources();
        }
    }
    textures_.clear();
}

bool TextureResources::CollectDescriptorInfos(
    const std::vector<EntityId>& requested_ids,
    std::vector<vk::DescriptorImageInfo>& out_infos,
    std::vector<EntityId>& out_ids) const
{
    out_infos.clear();
    out_ids.clear();

    for (auto texture_id : requested_ids)
    {
        const auto it = textures_.find(texture_id);
        if (it == textures_.end())
        {
            owner_.logger_->warn(
                "Texture id {} not available for descriptor binding.",
                texture_id);
            return false;
        }
        const auto* texture = it->second;
        if (!texture || !texture->HasGpuResources())
        {
            owner_.logger_->warn(
                "Texture id {} missing GPU resources.",
                texture_id);
            return false;
        }
        out_infos.push_back(texture->GetDescriptorInfo());
        out_ids.push_back(texture_id);
    }
    return true;
}

void TextureResources::UploadTexture(
    EntityId /*id*/,
    frame::vulkan::Texture& texture_interface)
{
    auto& mutable_texture =
        static_cast<frame::TextureInterface&>(texture_interface);
    const auto size = mutable_texture.GetSize();
    if (size.x == 0 || size.y == 0)
    {
        throw std::runtime_error("Texture has invalid size.");
    }

    const bool is_cubemap =
        texture_interface.GetViewType() == vk::ImageViewType::eCube;
    const auto& proto = texture_interface.GetData();
    const auto structure = proto.pixel_structure().value();
    const auto element_size = proto.pixel_element_size().value();

    const vk::Format image_format = [&] {
        switch (structure)
        {
        case frame::proto::PixelStructure::RGB_ALPHA:
        case frame::proto::PixelStructure::BGR_ALPHA:
            switch (element_size)
            {
            case frame::proto::PixelElementSize::BYTE:
                return vk::Format::eR8G8B8A8Unorm;
            case frame::proto::PixelElementSize::SHORT:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::HALF:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::FLOAT:
                return vk::Format::eR32G32B32A32Sfloat;
            default:
                break;
            }
            break;
        case frame::proto::PixelStructure::RGB:
        case frame::proto::PixelStructure::BGR:
            switch (element_size)
            {
            case frame::proto::PixelElementSize::BYTE:
                return vk::Format::eR8G8B8A8Unorm;
            case frame::proto::PixelElementSize::SHORT:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::HALF:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::FLOAT:
                return vk::Format::eR32G32B32A32Sfloat;
            default:
                break;
            }
            break;
        case frame::proto::PixelStructure::GREY:
        case frame::proto::PixelStructure::GREY_ALPHA:
            switch (element_size)
            {
            case frame::proto::PixelElementSize::BYTE:
                return vk::Format::eR8G8B8A8Unorm;
            case frame::proto::PixelElementSize::SHORT:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::HALF:
                return vk::Format::eR16G16B16A16Sfloat;
            case frame::proto::PixelElementSize::FLOAT:
                return vk::Format::eR32G32B32A32Sfloat;
            default:
                break;
            }
            break;
        case frame::proto::PixelStructure::DEPTH:
            switch (element_size)
            {
            case frame::proto::PixelElementSize::FLOAT:
                return vk::Format::eD32Sfloat;
            case frame::proto::PixelElementSize::SHORT:
                return vk::Format::eD16Unorm;
            default:
                break;
            }
            break;
        default:
            break;
        }
        throw std::runtime_error("Unsupported pixel format for Vulkan texture.");
    }();

    const bool is_depth =
        image_format == vk::Format::eD32Sfloat ||
        image_format == vk::Format::eD16Unorm;

    const std::size_t bytes_per_component =
        frame::vulkan::Texture::BytesPerComponent(element_size);
    const std::size_t component_count =
        frame::vulkan::Texture::ComponentCount(structure);

    if (bytes_per_component == 0 || component_count == 0)
    {
        throw std::runtime_error("Texture has unsupported component configuration.");
    }

    std::size_t src_stride = component_count * bytes_per_component;
    if (is_depth)
    {
        src_stride = bytes_per_component;
    }

    std::vector<std::uint8_t> src_data = mutable_texture.GetTextureByte();
    if (src_data.empty())
    {
        throw std::runtime_error("Texture contains no pixel data.");
    }

    const std::size_t face_count = is_cubemap ? 6 : 1;
    const std::size_t pixel_count =
        static_cast<std::size_t>(size.x) * size.y;
    const std::size_t pixel_count_total = pixel_count * face_count;
    const std::size_t expected_bytes = pixel_count_total * src_stride;

    if (!is_depth && src_data.size() < expected_bytes)
    {
        throw std::runtime_error("Texture pixel buffer is smaller than expected.");
    }

    std::vector<std::uint8_t> rgba_data;
    std::vector<std::uint8_t> linear_bytes;

    auto append_constant = [&](float value) {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        switch (element_size)
        {
        case frame::proto::PixelElementSize::BYTE: {
            const std::uint8_t converted =
                static_cast<std::uint8_t>(std::round(clamped * 255.0f));
            rgba_data.push_back(converted);
            break;
        }
        case frame::proto::PixelElementSize::SHORT: {
            const std::uint16_t converted =
                static_cast<std::uint16_t>(std::round(clamped * 65535.0f));
            const auto* ptr =
                reinterpret_cast<const std::uint8_t*>(&converted);
            rgba_data.insert(rgba_data.end(), ptr, ptr + sizeof(converted));
            break;
        }
        case frame::proto::PixelElementSize::HALF: {
            const std::uint16_t converted = glm::packHalf1x16(clamped);
            const auto* ptr =
                reinterpret_cast<const std::uint8_t*>(&converted);
            rgba_data.insert(rgba_data.end(), ptr, ptr + sizeof(converted));
            break;
        }
        case frame::proto::PixelElementSize::FLOAT: {
            const float converted = clamped;
            const auto* ptr = reinterpret_cast<const std::uint8_t*>(&converted);
            rgba_data.insert(rgba_data.end(), ptr, ptr + sizeof(converted));
            break;
        }
        default:
            throw std::runtime_error("Unsupported element size for Vulkan texture.");
        }
    };

    if (is_depth)
    {
        linear_bytes.resize(pixel_count_total * bytes_per_component);
        std::memcpy(linear_bytes.data(), src_data.data(), linear_bytes.size());
    }
    else
    {
        rgba_data.reserve(pixel_count_total * 4 * bytes_per_component);
        for (std::size_t i = 0; i < pixel_count_total; ++i)
        {
            const auto* pixel = &src_data[i * src_stride];
            auto push_channel = [&](std::size_t channel) {
                const auto* src =
                    &pixel[channel * bytes_per_component];
                rgba_data.insert(
                    rgba_data.end(),
                    src,
                    src + bytes_per_component);
            };

            switch (structure)
            {
            case frame::proto::PixelStructure::GREY:
            case frame::proto::PixelStructure::GREY_ALPHA: {
                push_channel(0);
                push_channel(0);
                push_channel(0);
                if (component_count > 1)
                {
                    push_channel(1);
                }
                else
                {
                    append_constant(1.0f);
                }
                break;
            }
            case frame::proto::PixelStructure::RGB:
            case frame::proto::PixelStructure::BGR: {
                push_channel(structure == frame::proto::PixelStructure::BGR ? 2 : 0);
                push_channel(1);
                push_channel(structure == frame::proto::PixelStructure::BGR ? 0 : 2);
                append_constant(1.0f);
                break;
            }
            case frame::proto::PixelStructure::RGB_ALPHA:
            case frame::proto::PixelStructure::BGR_ALPHA: {
                push_channel(structure == frame::proto::PixelStructure::BGR_ALPHA ? 2 : 0);
                push_channel(1);
                push_channel(structure == frame::proto::PixelStructure::BGR_ALPHA ? 0 : 2);
                push_channel(3);
                break;
            }
            default:
                throw std::runtime_error("Unsupported pixel structure for Vulkan texture.");
            }
        }
    }

    const std::vector<std::uint8_t>& upload_data =
        is_depth ? linear_bytes : rgba_data;
    if (upload_data.empty())
    {
        throw std::runtime_error("Texture upload buffer is empty.");
    }

    vk::UniqueDeviceMemory staging_memory;
    auto staging_buffer = owner_.gpu_memory_manager_->CreateBuffer(
        upload_data.size(),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_memory);
    void* mapped_data =
        owner_.vk_unique_device_->mapMemory(*staging_memory, 0, upload_data.size());
    std::memcpy(mapped_data, upload_data.data(), upload_data.size());
    owner_.vk_unique_device_->unmapMemory(*staging_memory);

    const std::uint32_t layer_count = static_cast<std::uint32_t>(face_count);

    vk::ImageCreateInfo image_info(
        is_cubemap ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags{},
        vk::ImageType::e2D,
        image_format,
        vk::Extent3D(size.x, size.y, 1),
        1,
        layer_count,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst |
            (is_depth ? vk::ImageUsageFlagBits::eDepthStencilAttachment
                      : vk::ImageUsageFlagBits::eSampled));

    auto image = owner_.vk_unique_device_->createImageUnique(image_info);
    auto requirements = owner_.vk_unique_device_->getImageMemoryRequirements(*image);
    vk::MemoryAllocateInfo allocate_info(
        requirements.size,
        owner_.gpu_memory_manager_->FindMemoryType(
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    auto image_memory = owner_.vk_unique_device_->allocateMemoryUnique(allocate_info);
    owner_.vk_unique_device_->bindImageMemory(*image, *image_memory, 0);

    owner_.command_queue_->TransitionImageLayout(
        *image,
        image_format,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        layer_count);
    owner_.command_queue_->CopyBufferToImage(
        *staging_buffer,
        *image,
        size.x,
        size.y,
        layer_count,
        upload_data.size() / layer_count);
    owner_.command_queue_->TransitionImageLayout(
        *image,
        image_format,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        layer_count);

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *image,
        texture_interface.GetViewType(),
        image_format,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, layer_count});
    auto view = owner_.vk_unique_device_->createImageViewUnique(view_info);

    vk::SamplerCreateInfo sampler_info(
        vk::SamplerCreateFlags{},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        is_cubemap ? vk::SamplerAddressMode::eClampToEdge : vk::SamplerAddressMode::eRepeat,
        is_cubemap ? vk::SamplerAddressMode::eClampToEdge : vk::SamplerAddressMode::eRepeat,
        is_cubemap ? vk::SamplerAddressMode::eClampToEdge : vk::SamplerAddressMode::eRepeat,
        0.0f,
        VK_FALSE,
        1.0f,
        VK_FALSE,
        vk::CompareOp::eAlways,
        0.0f,
        0.0f,
        vk::BorderColor::eIntOpaqueBlack,
        VK_FALSE);
    auto sampler = owner_.vk_unique_device_->createSamplerUnique(sampler_info);

    texture_interface.SetGpuResources(
        image_format,
        texture_interface.GetViewType(),
        std::move(image),
        std::move(image_memory),
        std::move(view),
        std::move(sampler));
}

} // namespace frame::vulkan
