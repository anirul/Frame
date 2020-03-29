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

	TEST_F(ImageTest, GetJPEGImageTest)
	{
		EXPECT_FALSE(image_);
		image_ = std::make_shared<sgl::Image>("../Asset/Planks/Color.jpg");
		EXPECT_TRUE(image_);
		auto pair = std::make_pair<size_t, size_t>(2048, 2048);
		for (const auto& pixel : *image_)
		{
			// R and G are suppose to be bigger than B.
			const float small_value = 0.1f;
			EXPECT_GT((pixel.x + pixel.y) / 2.f + small_value, pixel.z);
			// R and G are suppose to be kinda close.
			EXPECT_NEAR(0.125f, std::abs(pixel.x - pixel.y), 0.125f);
			EXPECT_FLOAT_EQ(1.0f, pixel.w);
		}
		EXPECT_EQ(pair, image_->GetSize());
	}

} // End namespace test.
