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
		const std::uint8_t* pixel = (std::uint8_t*)(image_->Data());
		for (int i = 0; i < image_->GetWidth() * image_->GetHeight(); ++i)
		{
			sgl::vector4 value = {
				static_cast<float>(pixel[i * 4 + 0]) / 255.0f,
				static_cast<float>(pixel[i * 4 + 1]) / 255.0f,
				static_cast<float>(pixel[i * 4 + 2]) / 255.0f,
				static_cast<float>(pixel[i * 4 + 3]) / 255.0f
			};
			// R and G are suppose to be bigger than B.
			const float small_value = 0.1f;
			EXPECT_GT((value.x + value.y) / 2.f + small_value, value.z);
			// R and G are suppose to be kinda close.
			EXPECT_NEAR(0.125f, std::abs(value.x - value.y), 0.125f);
			EXPECT_FLOAT_EQ(1.0f, value.w);
		}
		EXPECT_EQ(pair, image_->GetSize());
	}

} // End namespace test.
