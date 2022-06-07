#pragma once

#include "Frame/NameInterface.h"

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
	* @brief Get the size of the buffer (in bytes).
	* @return The size in bytes of the buffer.
	*/
    virtual std::size_t GetSize() const                                    = 0;
};

}  // End namespace frame.
