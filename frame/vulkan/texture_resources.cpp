#include "frame/vulkan/texture_resources.h"

#include <algorithm>
#include <chrono>
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

namespace
{

vk::Filter ToVkFilter(
    const frame::proto::TextureFilter& filter,
    vk::Filter fallback)
{
    switch (filter.value())
    {
    case frame::proto::TextureFilter::NEAREST:
    case frame::proto::TextureFilter::NEAREST_MIPMAP_NEAREST:
    case frame::proto::TextureFilter::NEAREST_MIPMAP_LINEAR:
        return vk::Filter::eNearest;
    case frame::proto::TextureFilter::LINEAR:
    case frame::proto::TextureFilter::LINEAR_MIPMAP_NEAREST:
    case frame::proto::TextureFilter::LINEAR_MIPMAP_LINEAR:
        return vk::Filter::eLinear;
    default:
        return fallback;
    }
}

vk::SamplerMipmapMode ToVkMipmapMode(
    const frame::proto::TextureFilter& filter,
    vk::SamplerMipmapMode fallback)
{
    switch (filter.value())
    {
    case frame::proto::TextureFilter::NEAREST_MIPMAP_NEAREST:
    case frame::proto::TextureFilter::LINEAR_MIPMAP_NEAREST:
        return vk::SamplerMipmapMode::eNearest;
    case frame::proto::TextureFilter::NEAREST_MIPMAP_LINEAR:
    case frame::proto::TextureFilter::LINEAR_MIPMAP_LINEAR:
        return vk::SamplerMipmapMode::eLinear;
    default:
        return fallback;
    }
}

vk::SamplerAddressMode ToVkAddressMode(
    const frame::proto::TextureFilter& filter,
    vk::SamplerAddressMode fallback)
{
    switch (filter.value())
    {
    case frame::proto::TextureFilter::CLAMP_TO_EDGE:
        return vk::SamplerAddressMode::eClampToEdge;
    case frame::proto::TextureFilter::MIRRORED_REPEAT:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case frame::proto::TextureFilter::REPEAT:
        return vk::SamplerAddressMode::eRepeat;
    case frame::proto::TextureFilter::CLAMP_TO_BORDER:
        return vk::SamplerAddressMode::eClampToBorder;
    default:
        return fallback;
    }
}

} // namespace

TextureResources::TextureResources(Device& owner)
    : owner_(owner)
{
}

void TextureResources::Build(
    LevelInterface& level,
    const frame::json::LevelData& level_data)
{
    Destroy();

    using Clock = std::chrono::steady_clock;
    const auto total_start = Clock::now();
    std::size_t uploaded_count = 0;

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

        const auto texture_start = Clock::now();
        try
        {
            UploadTexture(texture_id, *texture_ptr);
            textures_[texture_id] = texture_ptr;
            ++uploaded_count;
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    Clock::now() - texture_start);
            owner_.logger_->info(
                "Uploaded texture {} in {} ms.",
                proto_texture.name(),
                elapsed.count());
        }
        catch (const std::exception& ex)
        {
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    Clock::now() - texture_start);
            owner_.logger_->warn(
                "Skipping texture {} after {} ms: {}",
                proto_texture.name(),
                elapsed.count(),
                ex.what());
            texture_ptr->ResetGpuResources();
        }
    }

    const auto total_elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - total_start);
    owner_.logger_->info(
        "Uploaded {} textures in {} ms.",
        uploaded_count,
        total_elapsed.count());
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

    const auto& src_data = texture_interface.GetTextureData();
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
    const std::vector<std::uint8_t>* upload_data = nullptr;

    auto write_constant = [&](std::uint8_t* dst, float value) {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        switch (element_size)
        {
        case frame::proto::PixelElementSize::BYTE: {
            const std::uint8_t converted =
                static_cast<std::uint8_t>(std::round(clamped * 255.0f));
            std::memcpy(dst, &converted, sizeof(converted));
            break;
        }
        case frame::proto::PixelElementSize::SHORT: {
            const std::uint16_t converted =
                static_cast<std::uint16_t>(std::round(clamped * 65535.0f));
            std::memcpy(dst, &converted, sizeof(converted));
            break;
        }
        case frame::proto::PixelElementSize::HALF: {
            const std::uint16_t converted = glm::packHalf1x16(clamped);
            std::memcpy(dst, &converted, sizeof(converted));
            break;
        }
        case frame::proto::PixelElementSize::FLOAT: {
            const float converted = clamped;
            std::memcpy(dst, &converted, sizeof(converted));
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
        upload_data = &linear_bytes;
    }
    else if (structure == frame::proto::PixelStructure::RGB_ALPHA &&
             component_count == 4 &&
             src_stride == 4 * bytes_per_component)
    {
        upload_data = &src_data;
    }
    else
    {
        const std::size_t dst_stride = 4 * bytes_per_component;
        rgba_data.resize(pixel_count_total * dst_stride);
        for (std::size_t i = 0; i < pixel_count_total; ++i)
        {
            const auto* src = src_data.data() + i * src_stride;
            auto* dst = rgba_data.data() + i * dst_stride;
            auto copy_channel = [&](std::size_t dst_channel, std::size_t src_channel) {
                std::memcpy(
                    dst + dst_channel * bytes_per_component,
                    src + src_channel * bytes_per_component,
                    bytes_per_component);
            };

            switch (structure)
            {
            case frame::proto::PixelStructure::GREY: {
                copy_channel(0, 0);
                copy_channel(1, 0);
                copy_channel(2, 0);
                write_constant(dst + 3 * bytes_per_component, 1.0f);
                break;
            }
            case frame::proto::PixelStructure::GREY_ALPHA: {
                copy_channel(0, 0);
                copy_channel(1, 0);
                copy_channel(2, 0);
                copy_channel(3, 1);
                break;
            }
            case frame::proto::PixelStructure::RGB: {
                copy_channel(0, 0);
                copy_channel(1, 1);
                copy_channel(2, 2);
                write_constant(dst + 3 * bytes_per_component, 1.0f);
                break;
            }
            case frame::proto::PixelStructure::BGR: {
                copy_channel(0, 2);
                copy_channel(1, 1);
                copy_channel(2, 0);
                write_constant(dst + 3 * bytes_per_component, 1.0f);
                break;
            }
            case frame::proto::PixelStructure::RGB_ALPHA: {
                copy_channel(0, 0);
                copy_channel(1, 1);
                copy_channel(2, 2);
                copy_channel(3, 3);
                break;
            }
            case frame::proto::PixelStructure::BGR_ALPHA: {
                copy_channel(0, 2);
                copy_channel(1, 1);
                copy_channel(2, 0);
                copy_channel(3, 3);
                break;
            }
            default:
                throw std::runtime_error("Unsupported pixel structure for Vulkan texture.");
            }
        }
        upload_data = &rgba_data;
    }

    if (!upload_data || upload_data->empty())
    {
        throw std::runtime_error("Texture upload buffer is empty.");
    }

    vk::UniqueDeviceMemory staging_memory;
    auto staging_buffer = owner_.gpu_memory_manager_->CreateBuffer(
        upload_data->size(),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_memory);
    void* mapped_data =
        owner_.vk_unique_device_->mapMemory(
            *staging_memory, 0, upload_data->size());
    std::memcpy(mapped_data, upload_data->data(), upload_data->size());
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

    owner_.command_queue_->SubmitOneTime([&](vk::CommandBuffer command_buffer) {
        auto record_transition = [&](vk::ImageLayout old_layout,
                                     vk::ImageLayout new_layout) {
            vk::ImageMemoryBarrier barrier(
                {},
                {},
                old_layout,
                new_layout,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                *image,
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, layer_count});

            vk::PipelineStageFlags src_stage;
            vk::PipelineStageFlags dst_stage;

            if (old_layout == vk::ImageLayout::eUndefined &&
                new_layout == vk::ImageLayout::eTransferDstOptimal)
            {
                barrier.srcAccessMask = {};
                barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
                src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
                dst_stage = vk::PipelineStageFlagBits::eTransfer;
            }
            else if (old_layout == vk::ImageLayout::eTransferDstOptimal &&
                     new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
            {
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
                src_stage = vk::PipelineStageFlagBits::eTransfer;
                dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
            }
            else
            {
                throw std::runtime_error(
                    "Unsupported Vulkan image layout transition for texture upload.");
            }

            command_buffer.pipelineBarrier(
                src_stage,
                dst_stage,
                {},
                nullptr,
                nullptr,
                barrier);
        };

        record_transition(
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal);

        std::vector<vk::BufferImageCopy> copies;
        copies.reserve(layer_count);
        const std::size_t stride = upload_data->size() / layer_count;
        for (std::uint32_t layer = 0; layer < layer_count; ++layer)
        {
            copies.emplace_back(
                stride * layer,
                0,
                0,
                vk::ImageSubresourceLayers{
                    vk::ImageAspectFlagBits::eColor, 0, layer, 1},
                vk::Offset3D{0, 0, 0},
                vk::Extent3D{size.x, size.y, 1});
        }

        command_buffer.copyBufferToImage(
            *staging_buffer,
            *image,
            vk::ImageLayout::eTransferDstOptimal,
            copies);

        record_transition(
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);
    });

    vk::ImageViewCreateInfo view_info(
        vk::ImageViewCreateFlags{},
        *image,
        texture_interface.GetViewType(),
        image_format,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, layer_count});
    auto view = owner_.vk_unique_device_->createImageViewUnique(view_info);

    const vk::Filter fallback_filter = vk::Filter::eLinear;
    const vk::SamplerAddressMode fallback_address =
        is_cubemap ? vk::SamplerAddressMode::eClampToEdge
                   : vk::SamplerAddressMode::eRepeat;
    const vk::Filter min_filter = ToVkFilter(
        proto.min_filter(), fallback_filter);
    const vk::Filter mag_filter = ToVkFilter(
        proto.mag_filter(), fallback_filter);
    const vk::SamplerMipmapMode mipmap_mode = ToVkMipmapMode(
        proto.min_filter(), vk::SamplerMipmapMode::eNearest);
    const vk::SamplerAddressMode wrap_s = ToVkAddressMode(
        proto.wrap_s(), fallback_address);
    const vk::SamplerAddressMode wrap_t = ToVkAddressMode(
        proto.wrap_t(), fallback_address);

    vk::SamplerCreateInfo sampler_info(
        vk::SamplerCreateFlags{},
        min_filter,
        mag_filter,
        mipmap_mode,
        wrap_s,
        wrap_t,
        fallback_address,
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
