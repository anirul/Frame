#pragma once

#include <gtest/gtest.h>
#include "Frame/File/Image.h"

namespace test {

	class ImageTest : public testing::Test 
	{
	public:
		ImageTest() = default;

	protected:
		std::unique_ptr<frame::file::Image> image_;
	};

} // End namespace test.
