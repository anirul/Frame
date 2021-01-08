#include "FrameBufferTest.h"
#include "Frame/OpenGL/Texture.h"

namespace test {

	TEST_F(FrameBufferTest, CreateFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<frame::opengl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(FrameBufferTest, CheckIdFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<frame::opengl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		EXPECT_NE(0, frame_->GetId());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(FrameBufferTest, BindAttachErrorFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<frame::opengl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		frame::opengl::RenderBuffer render{};
		EXPECT_THROW(frame_->AttachRender(render), std::exception);
	}

	TEST_F(FrameBufferTest, BindAttachFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<frame::opengl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		frame::opengl::RenderBuffer render{};
		render.CreateStorage({ 1, 1 });
		frame_->AttachRender(render);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(FrameBufferTest, BindTextureFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<frame::opengl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		frame::opengl::RenderBuffer render{};
		render.CreateStorage({ 1, 1 });
		frame_->AttachRender(render);
		EXPECT_NO_THROW(error_.Display());
		auto texture = std::make_shared<frame::opengl::Texture>(
			std::make_pair(8, 8));
		frame_->AttachTexture(texture);
		EXPECT_NO_THROW(error_.Display());
	}

} // End namespace test.
