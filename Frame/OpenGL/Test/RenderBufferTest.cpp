#include "RenderBufferTest.h"

namespace test {

	TEST_F(RenderBufferTest, CreateRenderTest)
	{
		EXPECT_FALSE(render_);
		render_ = std::make_unique<frame::opengl::RenderBuffer>();
		EXPECT_TRUE(render_);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(RenderBufferTest, CheckIdRenderTest)
	{
		EXPECT_FALSE(render_);
		render_ = std::make_unique<frame::opengl::RenderBuffer>();
		EXPECT_TRUE(render_);
		EXPECT_NE(0, render_->GetId());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(RenderBufferTest, BindStorageRenderTest)
	{
		EXPECT_FALSE(render_);
		render_ = std::make_unique<frame::opengl::RenderBuffer>();
		EXPECT_TRUE(render_);
		render_->CreateStorage({ 32, 32 });
		EXPECT_NO_THROW(error_.Display());
	}

} // End namespace test.
