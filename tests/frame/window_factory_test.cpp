#include "frame/window_factory_test.h"

#include <stdexcept>

#include "frame/window_factory.h"

namespace test
{

TEST_F(WindowFactoryTest, CreateWindowOpenGLTest)
{
    EXPECT_FALSE(window_);
    EXPECT_NO_THROW(
        window_ = frame::CreateNewWindow(
            frame::DrawingTargetEnum::NONE, frame::RenderingAPIEnum::OPENGL));
    EXPECT_TRUE(window_);
}

TEST_F(WindowFactoryTest, CreateWindowVulkanTest)
{
    EXPECT_FALSE(window_);
    if (!frame::HasVulkanWindowFactory())
    {
        EXPECT_THROW(
            window_ = frame::CreateNewWindow(
                frame::DrawingTargetEnum::NONE,
                frame::RenderingAPIEnum::VULKAN),
            std::runtime_error);
        return;
    }
    EXPECT_NO_THROW(
        window_ = frame::CreateNewWindow(
            frame::DrawingTargetEnum::NONE, frame::RenderingAPIEnum::VULKAN));
    EXPECT_TRUE(window_);
}

} // End namespace test.
