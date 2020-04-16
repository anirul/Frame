#include "FrameTest.h"

namespace test {

	TEST_F(FrameTest, CreateFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::Frame>();
		EXPECT_TRUE(frame_);
		EXPECT_NO_THROW(error_->DisplayError());
	}

	TEST_F(FrameTest, CheckIdFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::Frame>();
		EXPECT_TRUE(frame_);
		EXPECT_NE(0, frame_->GetId());
		EXPECT_NO_THROW(error_->DisplayError());
	}

	TEST_F(FrameTest, BindAttachFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::Frame>();
		EXPECT_TRUE(frame_);
		sgl::Render render{};
		frame_->BindAttach(render);
		EXPECT_NO_THROW(error_->DisplayError());
	}

	TEST_F(FrameTest, BindTexture2DFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::Frame>();
		EXPECT_TRUE(frame_);
		sgl::Render render{};
		frame_->BindAttach(render);
		EXPECT_NO_THROW(error_->DisplayError());
		sgl::Texture texture("../Asset/Texture.tga");
		frame_->BindTexture2D(texture);
		EXPECT_NO_THROW(error_->DisplayError());
	}

} // End namespace test.
