#include "Buffer.h"
#include <exception>
#include <stdexcept>

namespace sgl {


	Buffer::Buffer(
		const BufferType buffer_type /*= BufferType::ARRAY_BUFFER*/, 
		const BufferUsage buffer_usage /*= BufferUsage::STATIC_DRAW*/) :
		buffer_type_(buffer_type),
		buffer_usage_(buffer_usage)
	{
		glGenBuffers(1, &buffer_object_);
	}
	
	Buffer::~Buffer()
	{
		glDeleteBuffers(1, &buffer_object_);
	}

	void Buffer::Bind() const
	{
		glBindBuffer(static_cast<GLenum>(buffer_type_), buffer_object_);
	}

	void Buffer::UnBind() const
	{
		glBindBuffer(static_cast<GLenum>(buffer_type_), 0);
	}

	void Buffer::BindCopy(
		const size_t size, 
		const void* data /*= nullptr*/) const
	{
		Bind();
		glBufferData(
			static_cast<GLenum>(buffer_type_),
			size,
			data,
			static_cast<GLenum>(buffer_usage_));
		UnBind();
	}

} // End namespace sgl.
