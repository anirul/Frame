#include "ImageTest.h"

namespace test {

	TEST_F(ImageTest, CreateImageTest)
	{
		EXPECT_FALSE(image_);
		image_ = std::make_shared<sgl::Image>("../Asset/Texture.tga");
		EXPECT_TRUE(image_);
	}

	TEST_F(ImageTest, FailedCreateImageTest)
	{
		EXPECT_FALSE(image_);
		EXPECT_THROW(
			image_ = 
				std::make_shared<sgl::Image>("../Asset/SimpleVertex.glsl"),
			std::runtime_error);
		EXPECT_FALSE(image_);
	}

	TEST_F(ImageTest, GetSizeImageTest)
	{
		EXPECT_FALSE(image_);
		image_ = std::make_shared<sgl::Image>("../Asset/Texture.tga");
		EXPECT_TRUE(image_);
		auto pair = std::make_pair<size_t, size_t>(256, 256);
		EXPECT_EQ(pair, image_->GetSize());
	}

} // End namespace test.
