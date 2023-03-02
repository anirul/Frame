#include "frame/opengl/renderer_test.h"

#include "frame/context.h"
#include "frame/level.h"

namespace test {

TEST_F(RendererTest, CreateRenderingTest) {
    ASSERT_FALSE(renderer_);
    ASSERT_TRUE(LoadDefaultLevel());
    renderer_ = std::make_unique<frame::opengl::Renderer>(
        frame::Context{ window_.get(), &window_->GetUniqueDevice(), level_.get() },
        glm::uvec4(0, 0, window_->GetSize().x, window_->GetSize().y));
    EXPECT_TRUE(renderer_);
}

TEST_F(RendererTest, RenderingDisplayTest) {
    ASSERT_FALSE(renderer_);
    ASSERT_TRUE(LoadDefaultLevel());
    renderer_ = std::make_unique<frame::opengl::Renderer>(
        frame::Context{ window_.get(), &window_->GetUniqueDevice(), level_.get() },
        glm::uvec4(0, 0, window_->GetSize().x, window_->GetSize().y));
    EXPECT_TRUE(renderer_);
    renderer_->Display();
}

}  // End namespace test.
