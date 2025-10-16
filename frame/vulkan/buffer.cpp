#include "frame/vulkan/buffer.h"

#include <cstring>
#include <stdexcept>

namespace frame::vulkan
{

void Buffer::Copy(std::size_t size, const void* data) const
{
    data_.resize(size);
    if (size == 0)
    {
        return;
    }
    if (data)
    {
        std::memcpy(data_.data(), data, size);
    }
    else
    {
        std::fill(data_.begin(), data_.end(), 0);
    }
}

void Buffer::Copy(const std::vector<float>& vector) const
{
    CopyBytes(
        vector.data(),
        vector.size() * sizeof(float),
        data_);
}

void Buffer::Copy(const std::vector<std::uint32_t>& vector) const
{
    CopyBytes(
        vector.data(),
        vector.size() * sizeof(std::uint32_t),
        data_);
}

void Buffer::Copy(const std::vector<std::uint8_t>& vector) const
{
    data_ = vector;
}

void Buffer::Clear() const
{
    std::fill(data_.begin(), data_.end(), 0);
}

std::size_t Buffer::GetSize() const
{
    return data_.size();
}

void Buffer::CopyBytes(
    const void* data, std::size_t size, std::vector<std::uint8_t>& dst)
{
    dst.resize(size);
    if (size == 0)
    {
        return;
    }
    if (!data)
    {
        std::fill(dst.begin(), dst.end(), 0);
        return;
    }
    std::memcpy(dst.data(), data, size);
}

std::unique_ptr<frame::BufferInterface> CreatePointBuffer(
    std::vector<float>&& vector)
{
    auto buffer = std::make_unique<Buffer>();
    buffer->Copy(vector);
    return buffer;
}

std::unique_ptr<frame::BufferInterface> CreateIndexBuffer(
    std::vector<std::uint32_t>&& vector)
{
    auto buffer = std::make_unique<Buffer>();
    buffer->Copy(vector);
    return buffer;
}

} // namespace frame::vulkan
