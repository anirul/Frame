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
				frame::file::FindDirectory("Asset/") + "CubeMap/PositiveX.png"));
		EXPECT_TRUE(texture_);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(TextureTest, GetSizeTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(texture_);
		EXPECT_NO_THROW(
			texture_ = frame::opengl::file::LoadTextureFromFile(
				frame::file::FindDirectory("Asset/") + "CubeMap/PositiveX.png"));
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
		EXPECT_NO_THROW(
			texture_ = frame::opengl::file::LoadTextureFromFile(
				frame::file::FindDirectory("Asset") + "/CubeMap/Hamarikyu.hdr",
				frame::proto::PixelElementSize_HALF()));
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
		EXPECT_NO_THROW(
			texture_ = frame::opengl::file::LoadCubeMapTextureFromFiles(
				std::array<std::string, 6>{
					frame::file::FindDirectory("Asset/") + "CubeMap/PositiveX.png",
					frame::file::FindDirectory("Asset/") + "CubeMap/NegativeX.png",
					frame::file::FindDirectory("Asset/") + "CubeMap/PositiveY.png",
					frame::file::FindDirectory("Asset/") + "CubeMap/NegativeY.png",
					frame::file::FindDirectory("Asset/") + "CubeMap/PositiveZ.png",
					frame::file::FindDirectory("Asset/") + "CubeMap/NegativeZ.png" 
					}));
		EXPECT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(TextureTest, CreateEquirectangularTextureTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(texture_);
		EXPECT_NO_THROW(
			texture_ = frame::opengl::file::LoadCubeMapTextureFromFile(
				frame::file::FindDirectory("Asset/") + "CubeMap/Hamarikyu.hdr",
				frame::proto::PixelElementSize_HALF()));
		EXPECT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(TextureTest, CreateInvalidTextureFromProtoTest)
	{
		frame::proto::Texture texture_proto;
		std::pair<std::uint32_t, std::uint32_t> size = { 512, 512 };
		EXPECT_FALSE(texture_);
		EXPECT_THROW(
			texture_ = frame::proto::ParseTexture(texture_proto, size),
			std::exception);
	}


	TEST_F(TextureTest, CreateTextureFromProtoCheckSizeTest)
	{
		frame::proto::Texture texture_proto;
		std::pair<std::uint32_t, std::uint32_t> size = { 512, 512 };
		*texture_proto.mutable_pixel_element_size() = 
			frame::proto::PixelElementSize_HALF();
		*texture_proto.mutable_pixel_structure() = 
			frame::proto::PixelStructure_RGB();
		frame::proto::Size size_proto{};
		size_proto.set_x(-2);
		size_proto.set_y(-2);
		*texture_proto.mutable_size() = size_proto;
		EXPECT_FALSE(texture_);
		EXPECT_NO_THROW(texture_ = 
			frame::proto::ParseTexture(texture_proto, size));
		EXPECT_TRUE(texture_);
		std::pair<std::uint32_t, std::uint32_t> test_size = { 256, 256 };
		EXPECT_EQ(test_size, texture_->GetSize());
	}

	TEST_F(TextureTest, CreateTextureFromProtoWrongSizeTest)
	{
		frame::proto::Texture texture_proto;
		std::pair<std::uint32_t, std::uint32_t> size = { 512, 512 };
		*texture_proto.mutable_pixel_element_size() =
			frame::proto::PixelElementSize_HALF();
		*texture_proto.mutable_pixel_structure() = 
			frame::proto::PixelStructure_RGB();
		frame::proto::Size size_proto{};
		size_proto.set_x(16);
		size_proto.set_y(16);
		*texture_proto.mutable_size() = size_proto;
		EXPECT_FALSE(texture_);
		EXPECT_NO_THROW(texture_ =
			frame::proto::ParseTexture(texture_proto, size));
		EXPECT_TRUE(texture_);
		EXPECT_NE(size, texture_->GetSize());
		std::pair<std::uint32_t, std::uint32_t> test_size = { 16, 16 };
		EXPECT_EQ(test_size, texture_->GetSize());
	}

} // End namespace test.
