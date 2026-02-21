#include "frame/vulkan/buffer_resources.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace frame::vulkan
{

BufferResourceManager::BufferResourceManager(
    vk::Device device,
    GpuMemoryManager& memory_manager,
    CommandQueue& command_queue,
    const Logger& logger)
    : device_(device),
      memory_manager_(&memory_manager),
      command_queue_(&command_queue),
      logger_(&logger)
{
}

void BufferResourceManager::Clear()
{
    storage_buffers_.clear();
    uniform_buffer_ = {};
}

BufferResource BufferResourceManager::MakeGpuBuffer(
    const std::string& name,
    const std::vector<std::uint8_t>& bytes,
    vk::BufferUsageFlags extra_flags)
{
    vk::UniqueDeviceMemory staging_memory;
    auto staging_buffer = memory_manager_->CreateBuffer(
        bytes.size(),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_memory);

    if (!bytes.empty())
    {
        void* mapped = device_.mapMemory(
            *staging_memory, 0, bytes.size());
        std::memcpy(mapped, bytes.data(), bytes.size());
        device_.unmapMemory(*staging_memory);
    }

    vk::UniqueDeviceMemory gpu_memory;
    auto gpu_buffer = memory_manager_->CreateBuffer(
        bytes.size(),
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eTransferSrc |
            extra_flags,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        gpu_memory);

    command_queue_->CopyBuffer(*staging_buffer, *gpu_buffer, bytes.size());

    BufferResource res{};
    res.name = name;
    res.size = bytes.size();
    res.buffer = std::move(gpu_buffer);
    res.memory = std::move(gpu_memory);
    return res;
}

void BufferResourceManager::BuildStorageBuffers(
    LevelInterface& level,
    const std::vector<EntityId>& buffer_ids)
{
    struct PendingUpload
    {
        vk::UniqueBuffer staging_buffer;
        vk::UniqueDeviceMemory staging_memory;
        vk::Buffer destination = VK_NULL_HANDLE;
        vk::DeviceSize size = 0;
    };

    storage_buffers_.clear();
    std::vector<PendingUpload> pending_uploads;
    pending_uploads.reserve(buffer_ids.size());

    for (auto buffer_id : buffer_ids)
    {
        try
        {
            const auto* buffer =
                dynamic_cast<const frame::vulkan::Buffer*>(
                    &level.GetBufferFromId(buffer_id));
            if (!buffer)
            {
                (*logger_)->warn(
                    "Buffer id {} is not a Vulkan buffer instance.",
                    buffer_id);
                continue;
            }

            const auto& bytes = buffer->GetRawData();
            if (bytes.empty())
            {
                (*logger_)->warn(
                    "Skipping empty storage buffer {}.",
                    level.GetNameFromId(buffer_id));
                continue;
            }

            PendingUpload upload{};
            upload.staging_buffer = memory_manager_->CreateBuffer(
                bytes.size(),
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent,
                upload.staging_memory);

            void* mapped = device_.mapMemory(
                *upload.staging_memory, 0, bytes.size());
            std::memcpy(mapped, bytes.data(), bytes.size());
            device_.unmapMemory(*upload.staging_memory);

            vk::UniqueDeviceMemory gpu_memory;
            auto gpu_buffer = memory_manager_->CreateBuffer(
                bytes.size(),
                vk::BufferUsageFlagBits::eTransferDst |
                    vk::BufferUsageFlagBits::eTransferSrc |
                    vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                gpu_memory);

            upload.destination = *gpu_buffer;
            upload.size = static_cast<vk::DeviceSize>(bytes.size());
            pending_uploads.push_back(std::move(upload));

            BufferResource res{};
            res.name = level.GetNameFromId(buffer_id);
            res.size = bytes.size();
            res.buffer = std::move(gpu_buffer);
            res.memory = std::move(gpu_memory);
            storage_buffers_.push_back(std::move(res));
        }
        catch (const std::exception& ex)
        {
            (*logger_)->warn(
                "Skipping buffer {}: {}",
                buffer_id,
                ex.what());
        }
    }

    if (!pending_uploads.empty())
    {
        command_queue_->SubmitOneTime(
            [&](vk::CommandBuffer command_buffer)
            {
                for (const auto& upload : pending_uploads)
                {
                    vk::BufferCopy copy_region(0, 0, upload.size);
                    command_buffer.copyBuffer(
                        *upload.staging_buffer,
                        upload.destination,
                        copy_region);
                }
            });
    }
}

void BufferResourceManager::BuildUniformBuffer(vk::DeviceSize size_bytes)
{
    if (size_bytes == 0)
    {
        throw std::runtime_error("Uniform buffer size must be non-zero.");
    }

    vk::UniqueDeviceMemory uniform_memory;
    auto uniform_buf = memory_manager_->CreateBuffer(
        size_bytes,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        uniform_memory);

    uniform_buffer_.name = "uniforms";
    uniform_buffer_.size = size_bytes;
    uniform_buffer_.buffer = std::move(uniform_buf);
    uniform_buffer_.memory = std::move(uniform_memory);
}

void BufferResourceManager::UpdateUniform(
    const void* data,
    std::size_t byte_count) const
{
    if (!uniform_buffer_.buffer ||
        !uniform_buffer_.memory ||
        uniform_buffer_.size == 0)
    {
        (*logger_)->warn("Uniform buffer not initialized before update.");
        return;
    }
    if (byte_count > static_cast<std::size_t>(uniform_buffer_.size))
    {
        (*logger_)->warn(
            "Uniform update size {} exceeds buffer capacity {}.",
            byte_count,
            static_cast<std::size_t>(uniform_buffer_.size));
        return;
    }
    void* mapped = device_.mapMemory(
        *uniform_buffer_.memory, 0, uniform_buffer_.size);
    if (data && byte_count > 0)
    {
        std::memcpy(mapped, data, byte_count);
    }
    else
    {
        std::memset(mapped, 0, uniform_buffer_.size);
    }
    device_.unmapMemory(*uniform_buffer_.memory);
}

void BufferResourceManager::LogCpuBufferSamples(
    LevelInterface& level,
    bool debug_dump_done) const
{
    if (debug_dump_done)
    {
        return;
    }
    for (const auto& res : storage_buffers_)
    {
        if (res.size < sizeof(float) * 16)
        {
            continue;
        }
        try
        {
            const auto& src_bytes =
                dynamic_cast<const frame::vulkan::Buffer&>(
                    level.GetBufferFromId(level.GetIdFromName(res.name)))
                    .GetRawData();
            if (src_bytes.size() < sizeof(float) * 16)
            {
                continue;
            }
            const float* f = reinterpret_cast<const float*>(src_bytes.data());
            (*logger_)->info(
                "Buffer {} sample floats: {:.3f} {:.3f} {:.3f} {:.3f} | {:.3f} {:.3f} {:.3f} {:.3f}",
                res.name,
                f[0], f[1], f[2], f[3],
                f[4], f[5], f[6], f[7]);
        }
        catch (const std::exception& ex)
        {
            (*logger_)->warn(
                "Failed to log buffer {} sample: {}",
                res.name,
                ex.what());
        }
    }
}

void BufferResourceManager::LogGpuBufferSamples() const
{
    if (!command_queue_)
    {
        return;
    }
    const vk::DeviceSize sample_bytes = 16 * sizeof(float);
    for (const auto& res : storage_buffers_)
    {
        if (!res.buffer || res.size < sizeof(float) * 8)
        {
            continue;
        }
        const vk::DeviceSize bytes_to_copy =
            std::min(res.size, sample_bytes);

        vk::UniqueDeviceMemory staging_memory;
        auto staging_buffer = memory_manager_->CreateBuffer(
            bytes_to_copy,
            vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            staging_memory);

        try
        {
            command_queue_->CopyBuffer(
                *res.buffer, *staging_buffer, bytes_to_copy);
            void* mapped = device_.mapMemory(
                *staging_memory, 0, bytes_to_copy);
            if (mapped)
            {
                const float* f = static_cast<const float*>(mapped);
                (*logger_)->info(
                    "GPU buffer {} sample floats: {:.3f} {:.3f} {:.3f} {:.3f} | {:.3f} {:.3f} {:.3f} {:.3f}",
                    res.name,
                    f[0], f[1], f[2], f[3],
                    f[4], f[5], f[6], f[7]);
                device_.unmapMemory(*staging_memory);
            }
        }
        catch (const std::exception& ex)
        {
            (*logger_)->warn(
                "Failed to read back GPU buffer {}: {}",
                res.name,
                ex.what());
        }
    }
}

} // namespace frame::vulkan
