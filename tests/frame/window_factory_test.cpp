#include "frame/window_factory_test.h"

#include "frame/window_factory.h"

namespace test {

TEST_F(WindowFactoryTest, CreateWindowOpenGLTest) {
  EXPECT_FALSE(window_);
  EXPECT_NO_THROW(window_ =
                      frame::CreateNewWindow(frame::DrawingTargetEnum::NONE,
                                             frame::RenderingAPIEnum::OPENGL));
  EXPECT_TRUE(window_);
}

TEST_F(WindowFactoryTest, CreateWindowVulkanTest) {
  EXPECT_FALSE(window_);
  EXPECT_NO_THROW(window_ =
                      frame::CreateNewWindow(frame::DrawingTargetEnum::NONE,
                                             frame::RenderingAPIEnum::VULKAN));
  EXPECT_TRUE(window_);
}

}  // End namespace test.
