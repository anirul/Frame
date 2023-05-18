#include "frame/opengl/buffer_test.h"

#include "frame/opengl/buffer.h"

namespace test {

TEST_F(BufferTest, CreationBufferTest) {
  EXPECT_FALSE(buffer_);
  buffer_ = std::make_unique<frame::opengl::Buffer>();
  EXPECT_TRUE(buffer_);
}

TEST_F(BufferTest, CheckIDBufferTest) {
  ASSERT_FALSE(buffer_);
  buffer_ = std::make_unique<frame::opengl::Buffer>();
  ASSERT_TRUE(buffer_);
  auto* gl_buffer = dynamic_cast<frame::opengl::Buffer*>(buffer_.get());
  ASSERT_TRUE(gl_buffer);
  EXPECT_NE(0, gl_buffer->GetId());
}

TEST_F(BufferTest, BindBufferTest) {
  ASSERT_FALSE(buffer_);
  buffer_ = std::make_unique<frame::opengl::Buffer>();
  ASSERT_TRUE(buffer_);
  auto* gl_buffer = dynamic_cast<frame::opengl::Buffer*>(buffer_.get());
  ASSERT_TRUE(gl_buffer);
  gl_buffer->Bind();
  gl_buffer->UnBind();
}

TEST_F(BufferTest, CopyBufferTest) {
  ASSERT_FALSE(buffer_);
  buffer_ = std::make_unique<frame::opengl::Buffer>();
  ASSERT_TRUE(buffer_);
  std::vector<float> test(4, 1.0f);
  auto* gl_buffer = dynamic_cast<frame::opengl::Buffer*>(buffer_.get());
  ASSERT_TRUE(gl_buffer);
  gl_buffer->Bind();
  buffer_->Copy(test.size() * sizeof(float), test.data());
  EXPECT_EQ(4 * sizeof(float), buffer_->GetSize());
  gl_buffer->UnBind();
  EXPECT_TRUE(buffer_);
}

}  // End namespace test.
