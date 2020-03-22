#include "TextureTest.h"

namespace test {

	TEST_F(TextureTest, CreateTextureTest)
	{
		EXPECT_FALSE(texture_);
		texture_ = std::make_shared<sgl::Texture>("../Asset/Texture.tga");
		EXPECT_TRUE(texture_);
	}

	TEST_F(TextureTest, GetSizeTextureTest)
	{
		ASSERT_FALSE(texture_);
		texture_ = std::make_shared<sgl::Texture>("../Asset/Texture.tga");
		ASSERT_TRUE(texture_);
		EXPECT_NE(0, texture_->GetId());
		auto pair = std::make_pair<size_t, size_t>(256, 256);
		EXPECT_EQ(pair, texture_->GetSize());
	}

	TEST_F(TextureTest, CreateTextureManagerTest)
	{
		EXPECT_FALSE(texture_manager_);
		texture_manager_ = std::make_shared<sgl::TextureManager>();
		EXPECT_TRUE(texture_manager_);
	}

	TEST_F(TextureTest, AddRemoveTextureManagerTest)
	{
		ASSERT_FALSE(texture_manager_);
		texture_manager_ = std::make_shared<sgl::TextureManager>();
		ASSERT_TRUE(texture_manager_);
		ASSERT_FALSE(texture_);
		texture_ = std::make_shared<sgl::Texture>("../Asset/Texture.tga");
		ASSERT_TRUE(texture_);
		EXPECT_TRUE(texture_manager_->AddTexture("texture1", texture_));
		EXPECT_TRUE(texture_manager_->RemoveTexture("texture1"));
	}

} // End namespace test.
