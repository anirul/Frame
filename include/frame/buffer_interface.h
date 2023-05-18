#pragma once

#include <vector>

#include "frame/name_interface.h"

namespace frame {

/**
 * @class BufferInterface
 * @brief The interface to a buffer class.
 */
struct BufferInterface : public NameInterface {
  /**
   * @brief Copy from a buffer and a size to the GPU memory.
   * @param size: In bytes of the memory.
   * @param data: Pointer to the memory.
   */
  virtual void Copy(const size_t size, const void* data = nullptr) const = 0;
  /**
   * @brief Copy a vector to a buffer.
   * @param vector: in vector to be copied in the buffer.
   */
  virtual void Copy(const std::vector<float>& vector) const = 0;
  /**
   * @brief Copy a vector to a buffer.
   * @param vector: in vector to be copied in the buffer.
   */
  virtual void Copy(const std::vector<std::uint32_t>& vector) const = 0;
  /**
   * @brief Copy a vector to a buffer.
   * @param vector: in vector to be copied in the buffer.
   */
  virtual void Copy(const std::vector<std::uint8_t>& vector) const = 0;
  /**
   * @brief Clear the buffer.
   */
  virtual void Clear() const = 0;
  /**
   * @brief Get the size of the buffer (in bytes).
   * @return The size in bytes of the buffer.
   */
  virtual std::size_t GetSize() const = 0;
};

}  // End namespace frame.
