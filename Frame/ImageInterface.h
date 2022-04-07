#pragma once

#include "Frame/Proto/Proto.h"

namespace frame {

    struct ImageInterface 
    {
        virtual ~ImageInterface() = default;
        virtual const std::pair<std::uint32_t, std::uint32_t> 
            GetSize() const = 0;
		virtual const int GetLength() const = 0;
		virtual const void* Data() const = 0;
		// Needed for the accessing of the pointer.
		virtual void* Data() = 0;
		virtual const proto::PixelElementSize GetPixelElementSize() const = 0;
		virtual const proto::PixelStructure GetPixelStructure() const = 0;
    };

} // End namespace frame.