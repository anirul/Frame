#include "frame/vulkan/window_test.h"

namespace test
{

TEST_F(VulkanWindowNoneTest, CreateWindowReturnsValidContext)
{
    EXPECT_NE(window().GetGraphicContext(), nullptr);
    EXPECT_NE(window().GetWindowContext(), nullptr);
    EXPECT_EQ(window().GetDrawingTargetEnum(), frame::DrawingTargetEnum::NONE);

    const auto result = window().Run([] { return true; });
    EXPECT_EQ(result, frame::WindowReturnEnum::UKNOWN);
}

TEST_F(VulkanDevicePluginTest, RemovePluginByNameRemovesMatchingPlugin)
{
    auto& device = window().GetDevice();

    auto plugin = std::make_unique<DummyVulkanPlugin>("PluginA");
    auto* pluginPtr = plugin.get();
    pluginPtr->AttachDevice(device);

    device.AddPlugin(std::move(plugin));
    EXPECT_EQ(device.GetPluginNames().size(), 1u);

    const glm::uvec2 resizedSize(128u, 64u);
    device.Resize(resizedSize);
    EXPECT_EQ(pluginPtr->lastStartupSize(), resizedSize);

    device.RemovePluginByName("PluginA");
    EXPECT_TRUE(device.GetPluginNames().empty());
}

} // namespace test
