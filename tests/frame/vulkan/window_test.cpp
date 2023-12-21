#include "frame/vulkan/window_test.h"

#include "frame/vulkan/window_factory.h"

namespace test
{

TEST_F(WindowTest, CreateWindowTest)
{
    EXPECT_FALSE(window_);
    window_ = frame::vulkan::CreateSDL2VulkanNone({640, 512});
    EXPECT_TRUE(window_);
}

TEST_F(WindowTest, GetSizeWindowTest)
{
    EXPECT_FALSE(window_);
    window_ = frame::vulkan::CreateSDL2VulkanNone({640, 512});
    EXPECT_TRUE(window_);
    glm::uvec2 pair = {640, 512};
    EXPECT_EQ(pair, window_->GetSize());
}

TEST_F(WindowTest, CreateDeviceWindowTest)
{
    EXPECT_FALSE(window_);
    window_ = frame::vulkan::CreateSDL2VulkanNone({640, 512});
    EXPECT_TRUE(window_);
    EXPECT_EQ(window_->GetDrawingTargetEnum(), frame::DrawingTargetEnum::NONE);
    EXPECT_EQ(
        window_->GetDevice().GetDeviceEnum(), frame::RenderingAPIEnum::VULKAN);
}

} // End namespace test.
