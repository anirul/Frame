#pragma once

#include <GL/glew.h>

namespace sgl {

	enum class BufferType
	{
		ARRAY_BUFFER				= GL_ARRAY_BUFFER,
		ATOMIC_COUNTER_BUFFER		= GL_ATOMIC_COUNTER_BUFFER,
		COPY_READ_BUFFER			= GL_COPY_READ_BUFFER,
		COPY_WRITE_BUFFER			= GL_COPY_WRITE_BUFFER,
		DISPATCH_INDIRECT_BUFFER	= GL_DISPATCH_INDIRECT_BUFFER,
		DRAW_INDIRECT_BUFFER		= GL_DRAW_INDIRECT_BUFFER,
		ELEMENT_ARRAY_BUFFER		= GL_ELEMENT_ARRAY_BUFFER,
		PIXEL_PACK_BUFFER			= GL_PIXEL_PACK_BUFFER,
		PIXEL_UNPACK_BUFFER			= GL_PIXEL_UNPACK_BUFFER,
		QUERY_BUFFER				= GL_QUERY_BUFFER,
		SHADER_STORAGE_BUFFER		= GL_SHADER_STORAGE_BUFFER,
		TEXTURE_BUFFER				= GL_TEXTURE_BUFFER,
		TRANSFORM_FEEDBACK_BUFFER	= GL_TRANSFORM_FEEDBACK_BUFFER,
		UNIFORM_BUFFER				= GL_UNIFORM_BUFFER,
	};

	enum class BufferUsage
	{
		STREAM_DRAW					= GL_STREAM_DRAW, 
		STREAM_READ					= GL_STREAM_READ, 
		STREAM_COPY					= GL_STREAM_COPY, 
		STATIC_DRAW					= GL_STATIC_DRAW, 
		STATIC_READ					= GL_STATIC_READ, 
		STATIC_COPY					= GL_STATIC_COPY, 
		DYNAMIC_DRAW				= GL_DYNAMIC_DRAW, 
		DYNAMIC_READ				= GL_DYNAMIC_READ, 
		DYNAMIC_COPY				= GL_DYNAMIC_COPY,
	};

	class Buffer 
	{
	public:
		Buffer(
			const BufferType buffer_type = BufferType::ARRAY_BUFFER, 
			const BufferUsage buffer_usage = BufferUsage::STATIC_DRAW);
		virtual ~Buffer();
		void BindCopy(const size_t size, const void* data = nullptr) const;
		void Bind() const;
		void UnBind() const;
		const unsigned int GetId() const { return buffer_object_; }

	private:
		const BufferType buffer_type_;
		const BufferUsage buffer_usage_;
		unsigned int buffer_object_ = 0;
	};

} // End namespace sgl.
