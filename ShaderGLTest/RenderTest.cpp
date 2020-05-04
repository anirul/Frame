#include "RenderTest.h"

namespace test {

	TEST_F(RenderTest, CreateRenderTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(render_);
		render_ = std::make_shared<sgl::Render>();
		EXPECT_TRUE(render_);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(RenderTest, CheckIdRenderTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(render_);
		render_ = std::make_shared<sgl::Render>();
		EXPECT_TRUE(render_);
		EXPECT_NE(0, render_->GetId());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(RenderTest, BindStorageRenderTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(render_);
		render_ = std::make_shared<sgl::Render>();
		EXPECT_TRUE(render_);
		render_->BindStorage({ 32, 32 });
		EXPECT_NO_THROW(error_.Display());
	}

} // End namespace test.
