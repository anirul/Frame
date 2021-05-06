#include "RenderingTest.h"
#include "Frame/Level.h"

namespace test {

	TEST_F(RenderingTest, CreateRenderingTest)
	{
		ASSERT_FALSE(renderer_);
		renderer_ = std::make_shared<frame::opengl::Renderer>(
			level_, 
			window_->GetSize());
		EXPECT_TRUE(renderer_);
	}

	TEST_F(RenderingTest, RenderingDisplayTest)
	{
		ASSERT_FALSE(renderer_);
		renderer_ = std::make_shared<frame::opengl::Renderer>(
			level_,
			window_->GetSize());
		EXPECT_TRUE(renderer_);
		renderer_->Display(window_->GetUniqueUniform().get());
	}

} // End namespace test.
