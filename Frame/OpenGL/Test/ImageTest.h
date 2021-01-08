#pragma once

#include <gtest/gtest.h>
#include "Frame/ShaderGLLib/Image.h"

namespace test {

	class ImageTest : public testing::Test
	{
	public:
		ImageTest() = default;

	protected:
		std::shared_ptr<sgl::Image> image_;
	};

} // End namespace test.
