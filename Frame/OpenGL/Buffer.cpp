#include "Buffer.h"

#include <exception>
#include <stdexcept>

namespace frame::opengl {

Buffer::Buffer(const BufferTypeEnum buffer_type /*= BufferTypeEnum::ARRAY_BUFFER*/,
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

void Buffer::Copy(const std::size_t size, const void* data /*= nullptr*/) const {
    Bind();
    glBufferData(static_cast<GLenum>(buffer_type_), size, data, static_cast<GLenum>(buffer_usage_));
    UnBind();
}

std::size_t Buffer::GetSize() const {
    std::size_t size[1] = { 0 };
    Bind();
    glGetBufferParameteriv(static_cast<GLenum>(buffer_type_), GL_BUFFER_SIZE, (GLint*)&size);
    UnBind();
    return size[0];
}

}  // End namespace frame::opengl.
