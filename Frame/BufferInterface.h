#pragma once

#include "Frame/NameInterface.h"

namespace frame {

	struct BufferInterface : public NameInterface
	{
		virtual void Copy(
			const size_t size, 
			const void* data = nullptr) const = 0;
		virtual std::size_t GetSize() const = 0;
	};

} // End namespace frame.
