#include "buffer.h"

#include <exception>
#include <stdexcept>

namespace frame::opengl {

Buffer::Buffer(
    const BufferTypeEnum buffer_type /*= BufferTypeEnum::ARRAY_BUFFER*/,
    const BufferUsageEnum buffer_usage /*= BufferUsageEnum::STATIC_DRAW*/)
    : buffer_type_(buffer_type), buffer_usage_(buffer_usage) {
  glGenBuffers(1, &buffer_object_);
}

Buffer::~Buffer() { glDeleteBuffers(1, &buffer_object_); }

void Buffer::Bind(const unsigned int slot /* = 0*/) const {
  if (locked_bind_) return;
  glBindBuffer(static_cast<GLenum>(buffer_type_), buffer_object_);
}

void Buffer::UnBind() const {
  if (locked_bind_) return;
  glBindBuffer(static_cast<GLenum>(buffer_type_), 0);
}

void Buffer::Copy(const std::size_t size,
                  const void* data /*= nullptr*/) const {
  Bind();
  glBufferData(static_cast<GLenum>(buffer_type_), size, data,
               static_cast<GLenum>(buffer_usage_));
  UnBind();
}

void Buffer::Copy(const std::vector<float>& vector) const {
  Bind();
  glBufferData(static_cast<GLenum>(buffer_type_), vector.size() * sizeof(float),
               vector.data(), static_cast<GLenum>(buffer_usage_));
  UnBind();
}

void Buffer::Copy(const std::vector<unsigned int>& vector) const {
  Bind();
  glBufferData(static_cast<GLenum>(buffer_type_),
               vector.size() * sizeof(unsigned int), vector.data(),
               static_cast<GLenum>(buffer_usage_));
  UnBind();
}

void Buffer::Copy(const std::vector<std::uint8_t>& vector) const {
  Bind();
  glBufferData(static_cast<GLenum>(buffer_type_),
               vector.size() * sizeof(std::uint8_t), vector.data(),
               static_cast<GLenum>(buffer_usage_));
  UnBind();
}

std::size_t Buffer::GetSize() const {
  std::size_t size[1] = {0};
  Bind();
  glGetBufferParameteriv(static_cast<GLenum>(buffer_type_), GL_BUFFER_SIZE,
                         reinterpret_cast<GLint*>(&size));
  UnBind();
  return size[0];
}

void Buffer::Clear() const {
  Bind();
  glClearBufferData(static_cast<GLenum>(buffer_type_), GL_R32UI, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, nullptr);
  UnBind();
}

std::unique_ptr<frame::BufferInterface> CreatePointBuffer(
    std::vector<float>&& vector) {
  auto point_buffer = std::make_unique<Buffer>();
  point_buffer->Copy(vector);
  return point_buffer;
}

std::unique_ptr<frame::BufferInterface> CreateIndexBuffer(
    std::vector<std::uint32_t>&& vector) {
  auto index_buffer =
      std::make_unique<Buffer>(BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
  index_buffer->Copy(vector);
  return index_buffer;
}

}  // End namespace frame::opengl.
