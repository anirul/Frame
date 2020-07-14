#include "TextureTest.h"

namespace test {

	TEST_F(TextureTest, CreateTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_);
		texture_ = std::make_shared<sgl::Texture>(
			"../Asset/CubeMap/PositiveX.png");
		EXPECT_TRUE(texture_);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(TextureTest, GetSizeTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(texture_);
		texture_ = std::make_shared<sgl::Texture>(
			"../Asset/CubeMap/PositiveX.png");
		ASSERT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		auto pair = std::make_pair<std::uint32_t, std::uint32_t>(1024, 1024);
		EXPECT_EQ(pair, texture_->GetSize());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(TextureTest, CreateHDRTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(texture_);
		texture_ =
			std::make_shared<sgl::Texture>(
				"../Asset/CubeMap/Hamarikyu.hdr",
				sgl::PixelElementSize::HALF);
		ASSERT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		auto pair = std::make_pair<std::uint32_t, std::uint32_t>(3200, 1600);
		EXPECT_EQ(pair, texture_->GetSize());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(TextureTest, CreateCubeMapTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_);
		texture_ = std::make_shared<sgl::TextureCubeMap>(
			std::array<std::string, 6>{
				"../Asset/CubeMap/PositiveX.png",
				"../Asset/CubeMap/NegativeX.png",
				"../Asset/CubeMap/PositiveY.png",
				"../Asset/CubeMap/NegativeY.png",
				"../Asset/CubeMap/PositiveZ.png",
				"../Asset/CubeMap/NegativeZ.png" });
		EXPECT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(TextureTest, CreateEquirectangularTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_);
		texture_ = std::make_shared<sgl::TextureCubeMap>(
			"../Asset/CubeMap/Hamarikyu.hdr");
		EXPECT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		EXPECT_NO_THROW(error_.Display());
	}

} // End namespace test.
