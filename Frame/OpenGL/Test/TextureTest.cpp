#include "TextureTest.h"
#include <GL/glew.h>
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/Proto/ParseTexture.h"

namespace test {

	TEST_F(TextureTest, CreateTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_);
		EXPECT_NO_THROW(
			texture_ = frame::opengl::file::LoadTextureFromFile(
				frame::file::FindFile("Asset/CubeMap/PositiveX.png")));
		EXPECT_TRUE(texture_);
	}

	TEST_F(TextureTest, GetSizeTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(texture_);
		EXPECT_NO_THROW(
			texture_ = frame::opengl::file::LoadTextureFromFile(
				frame::file::FindFile("Asset/CubeMap/PositiveX.png")));
		ASSERT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		auto pair = std::make_pair<std::uint32_t, std::uint32_t>(1024, 1024);
		EXPECT_EQ(pair, texture_->GetSize());
	}

	TEST_F(TextureTest, CreateHDRTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(texture_);
		EXPECT_NO_THROW(
			texture_ = frame::opengl::file::LoadTextureFromFile(
				frame::file::FindFile("Asset/CubeMap/Hamarikyu.hdr"),
				frame::proto::PixelElementSize_HALF()));
		ASSERT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		auto pair = std::make_pair<std::uint32_t, std::uint32_t>(3200, 1600);
		EXPECT_EQ(pair, texture_->GetSize());
	}

	TEST_F(TextureTest, CreateCubeMapTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_);
		EXPECT_NO_THROW(
			texture_ = frame::opengl::file::LoadCubeMapTextureFromFiles(
				std::array<std::string, 6>{
					frame::file::FindFile("Asset/CubeMap/PositiveX.png"),
					frame::file::FindFile("Asset/CubeMap/NegativeX.png"),
					frame::file::FindFile("Asset/CubeMap/PositiveY.png"),
					frame::file::FindFile("Asset/CubeMap/NegativeY.png"),
					frame::file::FindFile("Asset/CubeMap/PositiveZ.png"),
					frame::file::FindFile("Asset/CubeMap/NegativeZ.png") 
					}));
		EXPECT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
	}

	TEST_F(TextureTest, CreateEquirectangularTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_);
		EXPECT_NO_THROW(
			texture_ = frame::opengl::file::LoadCubeMapTextureFromFile(
				frame::file::FindFile("Asset/CubeMap/Hamarikyu.hdr"),
				frame::proto::PixelElementSize_HALF()));
		EXPECT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
	}

} // End namespace test.
