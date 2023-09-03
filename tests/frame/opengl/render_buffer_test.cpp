#include "frame/opengl/render_buffer_test.h"

namespace test {

    TEST_F(RenderBufferTest, CreateRenderTest) {
        EXPECT_FALSE(render_);
        render_ = std::make_unique<frame::opengl::RenderBuffer>();
        EXPECT_TRUE(render_);
    }

    TEST_F(RenderBufferTest, CheckIdRenderTest) {
        EXPECT_FALSE(render_);
        render_ = std::make_unique<frame::opengl::RenderBuffer>();
        EXPECT_TRUE(render_);
        EXPECT_NE(0, render_->GetId());
    }

    TEST_F(RenderBufferTest, BindStorageRenderTest) {
        EXPECT_FALSE(render_);
        render_ = std::make_unique<frame::opengl::RenderBuffer>();
        EXPECT_TRUE(render_);
        render_->CreateStorage({ 32, 32 });
    }

}  // End namespace test.
