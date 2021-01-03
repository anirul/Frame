#pragma once

#include "../Frame/BindInterface.h"

namespace frame {

	struct BufferInterface : public BindInterface
	{
		virtual void Copy(
			const size_t size, 
			const void* data = nullptr) const = 0;
		virtual const unsigned int GetId() const = 0;
	};

} // End namespace frame.
