#include "frame/vulkan/buffer.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace frame::vulkan
{

namespace
{

bool MatchesRawBytes(
    const std::vector<std::uint8_t>& raw_data,
    std::size_t size,
    const void* data)
{
    if (raw_data.size() != size)
    {
        return false;
    }
    if (size == 0)
    {
        return true;
    }
    if (!data)
    {
        return std::all_of(
            raw_data.begin(),
            raw_data.end(),
            [](std::uint8_t byte) { return byte == 0; });
    }
    return std::memcmp(raw_data.data(), data, size) == 0;
}

template <typename T>
bool MatchesRawVector(
    const std::vector<std::uint8_t>& raw_data,
    const std::vector<T>& vector)
{
    const std::size_t size = vector.size() * sizeof(T);
    if (raw_data.size() != size)
    {
        return false;
    }
    if (size == 0)
    {
        return true;
    }
    return std::memcmp(raw_data.data(), vector.data(), size) == 0;
}

void IncrementGeneration(std::uint64_t& generation)
{
    ++generation;
}

} // namespace

void Buffer::Copy(std::size_t size, const void* data) const
{
    if (MatchesRawBytes(data_, size, data))
    {
        return;
    }
    data_.resize(size);
    if (size == 0)
    {
        IncrementGeneration(generation_);
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
    IncrementGeneration(generation_);
}

void Buffer::Copy(const std::vector<float>& vector) const
{
    if (MatchesRawVector(data_, vector))
    {
        return;
    }
    CopyBytes(
        vector.data(),
        vector.size() * sizeof(float),
        data_);
    IncrementGeneration(generation_);
}

void Buffer::Copy(const std::vector<std::uint32_t>& vector) const
{
    if (MatchesRawVector(data_, vector))
    {
        return;
    }
    CopyBytes(
        vector.data(),
        vector.size() * sizeof(std::uint32_t),
        data_);
    IncrementGeneration(generation_);
}

void Buffer::Copy(const std::vector<std::uint8_t>& vector) const
{
    if (data_ == vector)
    {
        return;
    }
    data_ = vector;
    IncrementGeneration(generation_);
}

void Buffer::Clear() const
{
    const bool already_zero = std::all_of(
        data_.begin(),
        data_.end(),
        [](std::uint8_t byte) { return byte == 0; });
    if (already_zero)
    {
        return;
    }
    std::fill(data_.begin(), data_.end(), 0);
    IncrementGeneration(generation_);
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
