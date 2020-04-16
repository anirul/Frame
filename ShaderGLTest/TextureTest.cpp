#include "TextureTest.h"

namespace test {

	TEST_F(TextureTest, CreateTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_);
		texture_ = std::make_shared<sgl::Texture>("../Asset/Texture.tga");
		EXPECT_TRUE(texture_);
		EXPECT_NO_THROW(error_->DisplayError());
	}

	TEST_F(TextureTest, GetSizeTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(texture_);
		texture_ = std::make_shared<sgl::Texture>("../Asset/Texture.tga");
		ASSERT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		auto pair = std::make_pair<size_t, size_t>(256, 256);
		EXPECT_EQ(pair, texture_->GetSize());
		EXPECT_NO_THROW(error_->DisplayError());
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
		auto pair = std::make_pair<size_t, size_t>(3200, 1600);
		EXPECT_EQ(pair, texture_->GetSize());
		EXPECT_NO_THROW(error_->DisplayError());
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
		EXPECT_NO_THROW(error_->DisplayError());
	}

	TEST_F(TextureTest, CreateEquirectangularTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_);
		texture_ = std::make_shared<sgl::TextureCubeMap>(
			"../Asset/CubeMap/Hamarikyu.hdr");
		EXPECT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		EXPECT_NO_THROW(error_->DisplayError());
	}

	TEST_F(TextureTest, CreateTextureManagerTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_manager_);
		texture_manager_ = std::make_shared<sgl::TextureManager>();
		EXPECT_TRUE(texture_manager_);
		EXPECT_NO_THROW(error_->DisplayError());
	}

	TEST_F(TextureTest, AddRemoveTextureManagerTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(texture_manager_);
		texture_manager_ = std::make_shared<sgl::TextureManager>();
		ASSERT_TRUE(texture_manager_);
		ASSERT_FALSE(texture_);
		texture_ = std::make_shared<sgl::Texture>("../Asset/Texture.tga");
		ASSERT_TRUE(texture_);
		EXPECT_TRUE(texture_manager_->AddTexture("texture1", texture_));
		EXPECT_TRUE(texture_manager_->RemoveTexture("texture1"));
		EXPECT_NO_THROW(error_->DisplayError());
	}

} // End namespace test.
