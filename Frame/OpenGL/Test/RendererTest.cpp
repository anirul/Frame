#include "RenderingTest.h"
#include "Frame/Level.h"

namespace test {

	TEST_F(RendererTest, CreateRenderingTest)
	{
		ASSERT_FALSE(renderer_);
		renderer_ = std::make_shared<frame::opengl::Renderer>(
			level_.get(),
			window_->GetUniqueUniform().get(),
			window_->GetSize());
		EXPECT_TRUE(renderer_);
	}

	TEST_F(RendererTest, RenderingDisplayTest)
	{
		ASSERT_FALSE(renderer_);
		renderer_ = std::make_shared<frame::opengl::Renderer>(
			level_.get(),
			window_->GetUniqueUniform().get(),
			window_->GetSize());
		EXPECT_TRUE(renderer_);
		renderer_->Display();
	}

} // End namespace test.
