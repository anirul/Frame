#pragma once

#include <GL/glew.h>

#include <vector>

#include "frame/buffer_interface.h"
#include "frame/opengl/bind_interface.h"

namespace frame::opengl {

	enum class BufferTypeEnum : std::uint16_t {
		ARRAY_BUFFER = GL_ARRAY_BUFFER,
		ATOMIC_COUNTER_BUFFER = GL_ATOMIC_COUNTER_BUFFER,
		COPY_READ_BUFFER = GL_COPY_READ_BUFFER,
		COPY_WRITE_BUFFER = GL_COPY_WRITE_BUFFER,
		DISPATCH_INDIRECT_BUFFER = GL_DISPATCH_INDIRECT_BUFFER,
		DRAW_INDIRECT_BUFFER = GL_DRAW_INDIRECT_BUFFER,
		ELEMENT_ARRAY_BUFFER = GL_ELEMENT_ARRAY_BUFFER,
		PIXEL_PACK_BUFFER = GL_PIXEL_PACK_BUFFER,
		PIXEL_UNPACK_BUFFER = GL_PIXEL_UNPACK_BUFFER,
		QUERY_BUFFER = GL_QUERY_BUFFER,
		SHADER_STORAGE_BUFFER = GL_SHADER_STORAGE_BUFFER,
		TEXTURE_BUFFER = GL_TEXTURE_BUFFER,
		TRANSFORM_FEEDBACK_BUFFER = GL_TRANSFORM_FEEDBACK_BUFFER,
		UNIFORM_BUFFER = GL_UNIFORM_BUFFER,
	};

	enum class BufferUsageEnum : std::uint16_t {
		STREAM_DRAW = GL_STREAM_DRAW,
		STREAM_READ = GL_STREAM_READ,
		STREAM_COPY = GL_STREAM_COPY,
		STATIC_DRAW = GL_STATIC_DRAW,
		STATIC_READ = GL_STATIC_READ,
		STATIC_COPY = GL_STATIC_COPY,
		DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
		DYNAMIC_READ = GL_DYNAMIC_READ,
		DYNAMIC_COPY = GL_DYNAMIC_COPY,
	};

	/**
	 * @brief This is the main buffer class, this is here so hold of a single
	 * dimensional element (see vertex buffer and index buffers).
	 */
	class Buffer : public BindInterface, public BufferInterface {
	public:
		/**
		 * @brief Constructor this is where you can define the way a buffer is
		 * handled on the code level.
		 * @param buffer_type: This is the type of buffer you want to use
		 * (ARRAY_BUFFER should be enough for most cases).
		 * @param buffer_usage: Access way of the buffer is it gonna change or
		 * is it going to be the same for a large amount of time.
		 */
		Buffer(const BufferTypeEnum buffer_type = BufferTypeEnum::ARRAY_BUFFER,
			const BufferUsageEnum buffer_usage = BufferUsageEnum::STATIC_DRAW);
		virtual ~Buffer();

	public:
		/**
		 * @brief Copy a value in the buffer, the size is in bytes!
		 * @param size: Number of bytes to be copied.
		 * @param data: Data pointer to the data to be copied (void*).
		 */
		void Copy(
			const std::size_t size,
			const void* data = nullptr) const override;
		/**
		 * @brief Copy a vector to a buffer.
		 * @param vector: in vector to be copied in the buffer.
		 */
		void Copy(const std::vector<float>& vector) const override;
		/**
		 * @brief Copy a vector to a buffer.
		 * @param vector: in vector to be copied in the buffer.
		 */
		void Copy(const std::vector<unsigned int>& vector) const override;
		/**
		 * @brief Copy a vector to a buffer.
		 * @param vector: in vector to be copied in the buffer.
		 */
		void Copy(const std::vector<std::uint8_t>& vector) const override;
		/**
		 * @brief Clear the buffer.
		 */
		void Clear() const override;
		/**
		 * @brief Get the size in byte of the buffer.
		 * @return The size of the buffer.
		 */
		std::size_t GetSize() const override;
		/**
		 * @brief From the bind interface this is where we bind a buffer to the
		 * current context.
		 * @param slot: The slot of the current buffer to be bind in this case
		 * (buffer) this should be 0.
		 */
		void Bind(const unsigned int slot = 0) const override;
		/**
		 * @brief From the bind interface this is where we unbind the buffer
		 * from the current context.
		 */
		void UnBind() const override;

	public:
		/**
		 * @brief From the bind interface that lock the bind interface to the
		 * current, this is used by the scoped bind interface for more RAII.
		 */
		void LockedBind() const override { locked_bind_ = true; }
		/**
		 * @brief Same as before this is the unlock part suppose to be called by
		 * the scoped lock interface.
		 */
		void UnlockedBind() const override { locked_bind_ = false; }
		/**
		 * @brief From the bind interface this is suppose to return the object
		 * counter that allow OpenGL to manage object.
		 */
		unsigned int GetId() const override { return buffer_object_; }
		/**
		 * @brief From the name interface this is returning the name of the
		 * buffer.
		 * @return The name of the object.
		 */
		std::string GetName() const override { return name_; }
		/**
		 * @brief This is the set part of the name interface.
		 * @param name: The name of the object.
		 */
		void SetName(const std::string& name) override { name_ = name; }

	private:
		std::string name_ = "buffer???";
		mutable bool locked_bind_ = false;
		const BufferTypeEnum buffer_type_ = BufferTypeEnum::ARRAY_BUFFER;
		const BufferUsageEnum buffer_usage_ = BufferUsageEnum::STATIC_DRAW;
		unsigned int buffer_object_ = 0;
	};

	/**
	 * @brief Create a point buffer from a vector of floats.
	 * @param device: A pointer to a device.
	 * @param vector: A vector that is moved into the device and level.
	 */
	std::unique_ptr<BufferInterface> CreatePointBuffer(
		std::vector<float>&& vector);
	/**
	 * @brief Create an index buffer from a vector of unsigned integer.
	 * @param device: A pointer to a device.
	 * @param vector: A vector that is moved into the device and level.
	 */
	std::unique_ptr<BufferInterface> CreateIndexBuffer(
		std::vector<std::uint32_t>&& vector);

}  // End namespace frame::opengl.
