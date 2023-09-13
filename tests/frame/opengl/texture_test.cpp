#include "frame/opengl/texture_test.h"

#include <GL/glew.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_texture.h"
#include "frame/opengl/file/load_texture.h"
#include "frame/opengl/texture.h"

namespace test
{

TEST_F(TextureTest, CreateTextureTest)
{
    ASSERT_FALSE(texture_);
    std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
    EXPECT_NO_THROW(
        maybe_texture = frame::opengl::file::LoadTextureFromFile(
            frame::file::FindFile("asset/cubemap/positive_x.png")));
    ASSERT_TRUE(maybe_texture);
    texture_ = std::move(maybe_texture.value());
    ASSERT_TRUE(texture_);
}

TEST_F(TextureTest, GetSizeTextureByteTest)
{
    ASSERT_FALSE(texture_);
    std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
    EXPECT_NO_THROW(
        maybe_texture = frame::opengl::file::LoadTextureFromFile(
            frame::file::FindFile("asset/cubemap/positive_x.png")));
    ASSERT_TRUE(maybe_texture);
    texture_ = std::move(maybe_texture.value());
    ASSERT_TRUE(texture_);
    auto *opengl_texture =
        dynamic_cast<frame::opengl::Texture *>(texture_.get());
    EXPECT_NE(0, opengl_texture->GetId());
    auto pair = glm::uvec2(1024, 1024);
    EXPECT_EQ(pair, texture_->GetSize());
    auto vec8 = texture_->GetTextureByte();
    EXPECT_EQ(1024 * 1024 * 3, vec8.size());
    auto p = std::minmax_element(vec8.begin(), vec8.end());
    EXPECT_EQ(0x59, *p.first);
    EXPECT_EQ(0xff, *p.second);
}

TEST_F(TextureTest, CreateHDRTextureHalfTest)
{
    ASSERT_FALSE(texture_);
    std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
    EXPECT_NO_THROW(
        maybe_texture = frame::opengl::file::LoadTextureFromFile(
            frame::file::FindFile("asset/cubemap/hamarikyu.hdr"),
            frame::proto::PixelElementSize_HALF()));
    ASSERT_TRUE(maybe_texture);
    texture_ = std::move(maybe_texture.value());
    ASSERT_TRUE(texture_);
    auto *opengl_texture =
        dynamic_cast<frame::opengl::Texture *>(texture_.get());
    EXPECT_NE(0, opengl_texture->GetId());
    auto pair = glm::uvec2(3200, 1600);
    EXPECT_EQ(pair, texture_->GetSize());
    // TODO(anirul): Check half content in picture.
}

TEST_F(TextureTest, CreateHDRTextureFloatTest)
{
    ASSERT_FALSE(texture_);
    std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
    EXPECT_NO_THROW(
        maybe_texture = frame::opengl::file::LoadTextureFromFile(
            frame::file::FindFile("asset/cubemap/hamarikyu.hdr"),
            frame::proto::PixelElementSize_FLOAT()));
    ASSERT_TRUE(maybe_texture);
    texture_ = std::move(maybe_texture.value());
    ASSERT_TRUE(texture_);
    auto *opengl_texture =
        dynamic_cast<frame::opengl::Texture *>(texture_.get());
    EXPECT_NE(0, opengl_texture->GetId());
    auto pair = glm::uvec2(3200, 1600);
    EXPECT_EQ(pair, texture_->GetSize());
    auto vecf = texture_->GetTextureFloat();
    EXPECT_EQ(3200 * 1600 * 3, vecf.size());
    auto p = std::minmax_element(vecf.begin(), vecf.end());
    EXPECT_FLOAT_EQ(0.0f, *p.first);
    EXPECT_FLOAT_EQ(20.625f, *p.second);
}

} // End namespace test.
