#pragma once

#include <gtest/gtest.h>

#include <glm/glm.hpp>
#include <memory>
#include <stdexcept>
#include <string>

#include "frame/plugin_interface.h"
#include "frame/vulkan/window_factory.h"

namespace test
{

class DummyVulkanPlugin : public frame::PluginInterface
{
  public:
    explicit DummyVulkanPlugin(std::string name) : name_(std::move(name)) {}

    void Startup(glm::uvec2 size) override
    {
        lastStartupSize_ = size;
    }

    bool PollEvent(void* /*event*/) override
    {
        return false;
    }

    void PreRender(
        frame::UniformCollectionInterface& /*uniform*/,
        frame::DeviceInterface& /*device*/,
        frame::StaticMeshInterface& /*static_mesh*/,
        frame::MaterialInterface& /*material*/) override
    {
    }

    bool Update(frame::DeviceInterface& device, double /*dt*/ = 0.0) override
    {
        device_ = &device;
        return true;
    }

    frame::DeviceInterface& GetDevice() override
    {
        if (!device_)
        {
            throw std::runtime_error("Device not attached.");
        }
        return *device_;
    }

    void End() override {}

    std::string GetName() const override
    {
        return name_;
    }

    void SetName(const std::string& name) override
    {
        name_ = name;
    }

    void AttachDevice(frame::DeviceInterface& device)
    {
        device_ = &device;
    }

    glm::uvec2 lastStartupSize() const
    {
        return lastStartupSize_;
    }

  private:
    std::string name_;
    frame::DeviceInterface* device_ = nullptr;
    glm::uvec2 lastStartupSize_{0u, 0u};
};

class VulkanWindowNoneTest : public ::testing::Test
{
  protected:
    VulkanWindowNoneTest()
        : windowSize_(64u, 64u),
          window_(frame::vulkan::CreateSDLVulkanNone(windowSize_))
    {
        if (!window_)
        {
            throw std::runtime_error("Couldn't create Vulkan window.");
        }
    }

    frame::WindowInterface& window()
    {
        return *window_;
    }

    const glm::uvec2 windowSize_;
    std::unique_ptr<frame::WindowInterface> window_;
};

class VulkanDevicePluginTest : public VulkanWindowNoneTest
{
  protected:
    VulkanDevicePluginTest() = default;
};

} // namespace test
