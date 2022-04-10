#include "TextureTest.h"
#include <GL/glew.h>
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/Proto/ParseTexture.h"

namespace test {

	TEST_F(TextureTest, CreateTextureTest)
	{
		ASSERT_FALSE(texture_);
		std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
		EXPECT_NO_THROW(
			maybe_texture = frame::opengl::file::LoadTextureFromFile(
				frame::file::FindFile("Asset/CubeMap/PositiveX.png")));
		ASSERT_TRUE(maybe_texture);
		texture_ = std::move(maybe_texture.value());
		ASSERT_TRUE(texture_);
	}

	TEST_F(TextureTest, GetSizeTextureTest)
	{
		ASSERT_FALSE(texture_);
		std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
		EXPECT_NO_THROW(
			maybe_texture = frame::opengl::file::LoadTextureFromFile(
				frame::file::FindFile("Asset/CubeMap/PositiveX.png")));
		ASSERT_TRUE(maybe_texture);
		texture_ = std::move(maybe_texture.value());
		ASSERT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		auto pair = std::make_pair<std::uint32_t, std::uint32_t>(1024, 1024);
		EXPECT_EQ(pair, texture_->GetSize());
	}

	TEST_F(TextureTest, CreateHDRTextureTest)
	{
		ASSERT_FALSE(texture_);
		std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
		EXPECT_NO_THROW(
			maybe_texture = frame::opengl::file::LoadTextureFromFile(
				frame::file::FindFile("Asset/CubeMap/Hamarikyu.hdr"),
				frame::proto::PixelElementSize_HALF()));
		ASSERT_TRUE(maybe_texture);
		texture_ = std::move(maybe_texture.value());
		ASSERT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		auto pair = std::make_pair<std::uint32_t, std::uint32_t>(3200, 1600);
		EXPECT_EQ(pair, texture_->GetSize());
	}

} // End namespace test.
