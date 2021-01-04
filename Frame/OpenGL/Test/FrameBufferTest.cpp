#include "FrameBufferTest.h"

namespace test {

	TEST_F(FrameBufferTest, CreateFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(FrameBufferTest, CheckIdFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		EXPECT_NE(0, frame_->GetId());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(FrameBufferTest, BindAttachErrorFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		sgl::RenderBuffer render{};
		EXPECT_THROW(frame_->AttachRender(render), std::exception);
	}

	TEST_F(FrameBufferTest, BindAttachFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		sgl::RenderBuffer render{};
		render.CreateStorage({ 1, 1 });
		frame_->AttachRender(render);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(FrameBufferTest, BindTextureFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::FrameBuffer>();
		EXPECT_TRUE(frame_);
		sgl::RenderBuffer render{};
		render.CreateStorage({ 1, 1 });
		frame_->AttachRender(render);
		EXPECT_NO_THROW(error_.Display());
		sgl::Texture texture("../Asset/CubeMap/PositiveX.png");
		frame_->AttachTexture(texture);
		EXPECT_NO_THROW(error_.Display());
	}

} // End namespace test.
