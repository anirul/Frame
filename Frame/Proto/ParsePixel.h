#pragma once

#include "Frame/Proto/Proto.h"

namespace frame::proto {

	PixelElementSize PixelElementSize_BYTE();
	PixelElementSize PixelElementSize_SHORT();
	PixelElementSize PixelElementSize_HALF();
	PixelElementSize PixelElementSize_FLOAT();
	bool operator==(const PixelElementSize& l, const PixelElementSize& r);

	PixelStructure PixelStructure_GREY();
	PixelStructure PixelStructure_GREY_ALPHA();
	PixelStructure PixelStructure_RGB();
	PixelStructure PixelStructure_RGB_ALPHA();
	bool operator==(const PixelStructure& l, const PixelStructure& r);

} // End namespace frame::proto.