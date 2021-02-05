#include "Buffer.h"
#include <exception>
#include <stdexcept>

namespace frame::opengl {


	Buffer::Buffer(
		const BufferTypeEnum buffer_type /*= BufferTypeEnum::ARRAY_BUFFER*/, 
		const BufferUsageEnum buffer_usage /*= BufferUsageEnum::STATIC_DRAW*/) :
		buffer_type_(buffer_type),
		buffer_usage_(buffer_usage)
	{
		glGenBuffers(1, &buffer_object_);
		error_.Display(__FILE__, __LINE__ - 1);
	}
	
	Buffer::~Buffer()
	{
		glDeleteBuffers(1, &buffer_object_);
	}

	void Buffer::Bind(const unsigned int slot/* = 0*/) const
	{
		if (locked_bind_) return;
		glBindBuffer(static_cast<GLenum>(buffer_type_), buffer_object_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Buffer::UnBind() const
	{
		if (locked_bind_) return;
		glBindBuffer(static_cast<GLenum>(buffer_type_), 0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Buffer::Copy(
		const std::size_t size, 
		const void* data /*= nullptr*/) const
	{
		size_ = size;
		glBufferData(
			static_cast<GLenum>(buffer_type_),
			size,
			data,
			static_cast<GLenum>(buffer_usage_));
		error_.Display(__FILE__, __LINE__ - 5);
	}

} // End namespace frame::opengl.
