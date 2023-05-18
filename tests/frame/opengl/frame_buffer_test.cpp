#include "frame/opengl/frame_buffer_test.h"

#include "frame/opengl/texture.h"

namespace test {

TEST_F(FrameBufferTest, CreateFrameTest) {
  EXPECT_FALSE(frame_);
  frame_ = std::make_unique<frame::opengl::FrameBuffer>();
  EXPECT_TRUE(frame_);
}

TEST_F(FrameBufferTest, CheckIdFrameTest) {
  EXPECT_FALSE(frame_);
  frame_ = std::make_unique<frame::opengl::FrameBuffer>();
  EXPECT_TRUE(frame_);
  EXPECT_NE(0, frame_->GetId());
}

TEST_F(FrameBufferTest, BindAttachErrorFrameTest) {
  EXPECT_FALSE(frame_);
  frame_ = std::make_unique<frame::opengl::FrameBuffer>();
  EXPECT_TRUE(frame_);
  frame::opengl::RenderBuffer render{};
  EXPECT_THROW(frame_->AttachRender(render), std::exception);
}

TEST_F(FrameBufferTest, BindAttachFrameTest) {
  EXPECT_FALSE(frame_);
  frame_ = std::make_unique<frame::opengl::FrameBuffer>();
  EXPECT_TRUE(frame_);
  frame::opengl::RenderBuffer render{};
  render.CreateStorage({1, 1});
  EXPECT_NO_THROW(frame_->AttachRender(render));
}

TEST_F(FrameBufferTest, BindTextureFrameTest) {
  EXPECT_FALSE(frame_);
  frame_ = std::make_unique<frame::opengl::FrameBuffer>();
  EXPECT_TRUE(frame_);
  frame::opengl::RenderBuffer render{};
  render.CreateStorage({1, 1});
  EXPECT_NO_THROW(frame_->AttachRender(render));
  frame::TextureParameter texture_parameter = {};
  texture_parameter.size = {8, 8};
  frame::opengl::Texture texture(texture_parameter);
  EXPECT_NO_THROW(frame_->AttachTexture(texture.GetId()));
}

}  // End namespace test.
