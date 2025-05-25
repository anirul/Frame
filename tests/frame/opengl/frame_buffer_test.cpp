#include "frame/opengl/frame_buffer_test.h"

#include "frame/json/serialize_uniform.h"
#include "frame/opengl/texture.h"

namespace test
{

TEST_F(FrameBufferTest, CreateFrameTest)
{
    EXPECT_FALSE(frame_);
    frame_ = std::make_unique<frame::opengl::FrameBuffer>();
    EXPECT_TRUE(frame_);
}

TEST_F(FrameBufferTest, CheckIdFrameTest)
{
    EXPECT_FALSE(frame_);
    frame_ = std::make_unique<frame::opengl::FrameBuffer>();
    EXPECT_TRUE(frame_);
    EXPECT_NE(0, frame_->GetId());
}

TEST_F(FrameBufferTest, BindAttachErrorFrameTest)
{
    EXPECT_FALSE(frame_);
    frame_ = std::make_unique<frame::opengl::FrameBuffer>();
    EXPECT_TRUE(frame_);
    frame::opengl::RenderBuffer render{};
    EXPECT_THROW(frame_->AttachRender(render), std::exception);
}

TEST_F(FrameBufferTest, BindAttachFrameTest)
{
    EXPECT_FALSE(frame_);
    frame_ = std::make_unique<frame::opengl::FrameBuffer>();
    EXPECT_TRUE(frame_);
    frame::opengl::RenderBuffer render{};
    render.CreateStorage({1, 1});
    EXPECT_NO_THROW(frame_->AttachRender(render));
}

TEST_F(FrameBufferTest, BindTextureFrameTest)
{
    EXPECT_FALSE(frame_);
    frame_ = std::make_unique<frame::opengl::FrameBuffer>();
    EXPECT_TRUE(frame_);
    frame::opengl::RenderBuffer render{};
    render.CreateStorage({1, 1});
    EXPECT_NO_THROW(frame_->AttachRender(render));
    frame::proto::Texture proto_texture;
    proto_texture.mutable_size()->CopyFrom(frame::json::SerializeSize({8, 8}));
    proto_texture.mutable_pixel_element_size()->CopyFrom(
        frame::json::PixelElementSize_BYTE());
    proto_texture.mutable_pixel_structure()->CopyFrom(
        frame::json::PixelStructure_BGR());
    frame::opengl::Texture texture(proto_texture, glm::uvec2{1, 1});
    EXPECT_NO_THROW(frame_->AttachTexture(texture.GetId()));
}

} // End namespace test.
