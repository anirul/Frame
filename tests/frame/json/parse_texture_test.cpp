#include "frame/json/parse_texture_test.h"

#include <glm/common.hpp>

#include "frame/json/parse_pixel.h"
#include "frame/json/parse_texture.h"
#include "frame/json/proto.h"

namespace test
{

TEST_F(ParseTextureTest, CreateInvalidTextureFromProtoTest)
{
    frame::proto::Texture texture_proto;
    glm::uvec2 size = {512, 512};
    EXPECT_FALSE(texture_);
    std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
    EXPECT_THROW(
        maybe_texture = frame::json::ParseTexture(texture_proto, size),
        std::exception);
}

TEST_F(ParseTextureTest, CreateTextureFromProtoCheckSizeTest)
{
    frame::proto::Texture texture_proto;
    glm::uvec2 size = {512, 512};
    *texture_proto.mutable_pixel_element_size() =
        frame::json::PixelElementSize_HALF();
    *texture_proto.mutable_pixel_structure() =
        frame::json::PixelStructure_RGB();
    frame::proto::Size size_proto{};
    size_proto.set_x(-2);
    size_proto.set_y(-2);
    *texture_proto.mutable_size() = size_proto;
    EXPECT_FALSE(texture_);
    std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
    EXPECT_NO_THROW(
        maybe_texture = frame::json::ParseTexture(texture_proto, size));
    EXPECT_TRUE(maybe_texture);
    texture_ = std::move(maybe_texture.value());
    EXPECT_TRUE(texture_);
    glm::uvec2 test_size = {256, 256};
    EXPECT_EQ(test_size, texture_->GetSize());
}

TEST_F(ParseTextureTest, CreateTextureFromProtoWrongSizeTest)
{
    frame::proto::Texture texture_proto;
    glm::uvec2 size = {512, 512};
    *texture_proto.mutable_pixel_element_size() =
        frame::json::PixelElementSize_HALF();
    *texture_proto.mutable_pixel_structure() =
        frame::json::PixelStructure_RGB();
    frame::proto::Size size_proto{};
    size_proto.set_x(16);
    size_proto.set_y(16);
    *texture_proto.mutable_size() = size_proto;
    EXPECT_FALSE(texture_);
    std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
    EXPECT_NO_THROW(
        maybe_texture = frame::json::ParseTexture(texture_proto, size));
    texture_ = std::move(maybe_texture.value());
    EXPECT_TRUE(texture_);
    EXPECT_NE(size, texture_->GetSize());
    glm::uvec2 test_size = {16, 16};
    EXPECT_EQ(test_size, texture_->GetSize());
}

} // End namespace test.
