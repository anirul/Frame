#include "ImageTest.h"
#include <glm/glm.hpp>

namespace test {

	TEST_F(ImageTest, CreateImageTest)
	{
		EXPECT_FALSE(image_);
		image_ =
			std::make_shared<sgl::Image>("../Asset/CubeMap/PositiveX.png");
		EXPECT_TRUE(image_);
	}

	TEST_F(ImageTest, FailedCreateImageTest)
	{
		EXPECT_FALSE(image_);
		EXPECT_THROW(
			image_ = std::make_shared<sgl::Image>("../Asset/SimpleVertex.glsl"),
			std::runtime_error);
		EXPECT_FALSE(image_);
	}

	TEST_F(ImageTest, GetSizeImageTest)
	{
		EXPECT_FALSE(image_);
		image_ = std::make_shared<sgl::Image>("../Asset/CubeMap/PositiveX.png");
		EXPECT_TRUE(image_);
		auto pair = std::make_pair<int, int>(1024, 1024);
		EXPECT_EQ(pair, image_->GetSize());
	}

	TEST_F(ImageTest, GetLengthImageTest)
	{
		EXPECT_FALSE(image_);
		image_ = std::make_shared<sgl::Image>("../Asset/CubeMap/PositiveX.png");
		EXPECT_TRUE(image_);
		auto pair = std::make_pair<int, int>(1024, 1024);
		EXPECT_EQ(pair.first * pair.second, image_->GetLength());
	}

	TEST_F(ImageTest, GetJPEGImageTest)
	{
		EXPECT_FALSE(image_);
		image_ = 
			std::make_shared<sgl::Image>(
				"../Asset/Planks/Color.jpg", 
				sgl::PixelElementSize::BYTE,
				sgl::PixelStructure::RGB_ALPHA);
		EXPECT_TRUE(image_);
		const std::uint8_t* pixel = (std::uint8_t*)(image_->Data());
		for (int i = 0; i < image_->GetLength(); ++i)
		{
			glm::vec4 value = {
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
		auto pair = std::make_pair<int, int>(2048, 2048);
		EXPECT_EQ(pair, image_->GetSize());
	}

	TEST_F(ImageTest, LoadHDRImageTest)
	{
		EXPECT_FALSE(image_);
		image_ = 
			std::make_shared<sgl::Image>(
				"../Asset/CubeMap/Hamarikyu.hdr",
				sgl::PixelElementSize::FLOAT,
				sgl::PixelStructure::RGB_ALPHA);
		EXPECT_TRUE(image_);
		EXPECT_EQ(sgl::PixelElementSize::FLOAT, image_->GetPixelElementSize());
		EXPECT_EQ(sgl::PixelStructure::RGB_ALPHA, image_->GetPixelStructure());
		// Cast to the correct value.
		const float* ptr = static_cast<const float*>(image_->Data());
		for (int i = 0; i < image_->GetLength(); ++i)
		{
			// Check the values of colors are included between [0, 21.0[.
			for (int j : {0, 1, 2})
			{
				EXPECT_LE(0.0f, ptr[i * 4 + j]);
				EXPECT_GT(21.0f, ptr[i * 4 + j]);
			}
			// Check that the alpha value is equal to 1.0f (fully opaque).
			EXPECT_FLOAT_EQ(1.0f, ptr[i * 4 + 3]);
		}
	}

} // End namespace test.
