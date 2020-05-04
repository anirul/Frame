#include "FrameTest.h"

namespace test {

	TEST_F(FrameTest, CreateFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::Frame>();
		EXPECT_TRUE(frame_);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(FrameTest, CheckIdFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::Frame>();
		EXPECT_TRUE(frame_);
		EXPECT_NE(0, frame_->GetId());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(FrameTest, BindAttachFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::Frame>();
		EXPECT_TRUE(frame_);
		sgl::Render render{};
		frame_->BindAttach(render);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(FrameTest, BindTextureFrameTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(frame_);
		frame_ = std::make_shared<sgl::Frame>();
		EXPECT_TRUE(frame_);
		sgl::Render render{};
		frame_->BindAttach(render);
		EXPECT_NO_THROW(error_.Display());
		sgl::Texture texture("../Asset/CubeMap/PositiveX.png");
		frame_->BindTexture(texture);
		EXPECT_NO_THROW(error_.Display());
	}

} // End namespace test.
