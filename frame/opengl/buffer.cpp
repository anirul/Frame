#include "buffer.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <stdexcept>

namespace frame::opengl
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

Buffer::Buffer(
    const BufferTypeEnum buffer_type /*= BufferTypeEnum::ARRAY_BUFFER*/,
    const BufferUsageEnum buffer_usage /*= BufferUsageEnum::STATIC_DRAW*/)
    : buffer_type_(buffer_type), buffer_usage_(buffer_usage)
{
    glGenBuffers(1, &buffer_object_);
}

Buffer::~Buffer()
{
    glDeleteBuffers(1, &buffer_object_);
}

void Buffer::Bind(const unsigned int slot /* = 0*/) const
{
    if (locked_bind_)
        return;
    glBindBuffer(static_cast<GLenum>(buffer_type_), buffer_object_);
}

void Buffer::UnBind() const
{
    if (locked_bind_)
        return;
    glBindBuffer(static_cast<GLenum>(buffer_type_), 0);
}

void Buffer::BindBase(int binding) const
{
    glBindBufferBase(
        static_cast<GLenum>(buffer_type_), binding, buffer_object_);
}

void Buffer::Copy(const std::size_t size, const void* data /*= nullptr*/) const
{
    if (MatchesRawBytes(raw_data_, size, data))
    {
        return;
    }
    raw_data_.assign(size, 0);
    if (data && size > 0)
    {
        std::memcpy(raw_data_.data(), data, size);
    }
    Bind();
    glBufferData(
        static_cast<GLenum>(buffer_type_),
        size,
        data,
        static_cast<GLenum>(buffer_usage_));
    UnBind();
    IncrementGeneration(generation_);
}

void Buffer::Copy(const std::vector<float>& vector) const
{
    if (MatchesRawVector(raw_data_, vector))
    {
        return;
    }
    raw_data_.resize(vector.size() * sizeof(float));
    if (!vector.empty())
    {
        std::memcpy(
            raw_data_.data(),
            vector.data(),
            raw_data_.size());
    }
    Bind();
    glBufferData(
        static_cast<GLenum>(buffer_type_),
        vector.size() * sizeof(float),
        vector.data(),
        static_cast<GLenum>(buffer_usage_));
    UnBind();
    IncrementGeneration(generation_);
}

void Buffer::Copy(const std::vector<unsigned int>& vector) const
{
    if (MatchesRawVector(raw_data_, vector))
    {
        return;
    }
    raw_data_.resize(vector.size() * sizeof(unsigned int));
    if (!vector.empty())
    {
        std::memcpy(
            raw_data_.data(),
            vector.data(),
            raw_data_.size());
    }
    Bind();
    glBufferData(
        static_cast<GLenum>(buffer_type_),
        vector.size() * sizeof(unsigned int),
        vector.data(),
        static_cast<GLenum>(buffer_usage_));
    UnBind();
    IncrementGeneration(generation_);
}

void Buffer::Copy(const std::vector<std::uint8_t>& vector) const
{
    if (raw_data_ == vector)
    {
        return;
    }
    raw_data_ = vector;
    Bind();
    glBufferData(
        static_cast<GLenum>(buffer_type_),
        vector.size() * sizeof(std::uint8_t),
        vector.data(),
        static_cast<GLenum>(buffer_usage_));
    UnBind();
    IncrementGeneration(generation_);
}

std::size_t Buffer::GetSize() const
{
    std::size_t size[1] = {0};
    Bind();
    glGetBufferParameteriv(
        static_cast<GLenum>(buffer_type_),
        GL_BUFFER_SIZE,
        reinterpret_cast<GLint*>(&size));
    UnBind();
    return size[0];
}

void Buffer::Clear() const
{
    const bool already_zero = std::all_of(
        raw_data_.begin(),
        raw_data_.end(),
        [](std::uint8_t byte) { return byte == 0; });
    if (already_zero)
    {
        return;
    }
    std::fill(raw_data_.begin(), raw_data_.end(), 0);
    Bind();
    glClearBufferData(
        static_cast<GLenum>(buffer_type_),
        GL_R32UI,
        GL_RED_INTEGER,
        GL_UNSIGNED_INT,
        nullptr);
    UnBind();
    IncrementGeneration(generation_);
}

std::unique_ptr<frame::BufferInterface> CreatePointBuffer(
    std::vector<float>&& vector)
{
    auto point_buffer = std::make_unique<Buffer>();
    point_buffer->Copy(vector);
    return point_buffer;
}

std::unique_ptr<frame::BufferInterface> CreateIndexBuffer(
    std::vector<std::uint32_t>&& vector)
{
    auto index_buffer =
        std::make_unique<Buffer>(BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
    index_buffer->Copy(vector);
    return index_buffer;
}

} // End namespace frame::opengl.
