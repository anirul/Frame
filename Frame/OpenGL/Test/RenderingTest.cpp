#include "RenderingTest.h"
#include "Frame/Level.h"

namespace test {

	TEST_F(RenderingTest, CreateRenderingTest)
	{
		ASSERT_FALSE(rendering_);
		rendering_ = std::make_shared<frame::opengl::Rendering>(
			level_, 
			window_->GetSize());
		EXPECT_TRUE(rendering_);
	}

	TEST_F(RenderingTest, RenderingDisplayTest)
	{
		ASSERT_FALSE(rendering_);
		rendering_ = std::make_shared<frame::opengl::Rendering>(
			level_,
			window_->GetSize());
		EXPECT_TRUE(rendering_);
		rendering_->Display(window_->GetUniqueUniform().get());
	}

} // End namespace test.
