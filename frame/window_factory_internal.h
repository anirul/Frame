#pragma once

#include "frame/backend_internal.h"
#include "frame/window_interface.h"

namespace frame::internal
{

struct VulkanWindowFactory
{
    using FactoryFn = std::unique_ptr<WindowInterface> (*)(glm::uvec2);
    FactoryFn create_window = nullptr;
    FactoryFn create_none = nullptr;
};

void RegisterVulkanWindowFactory(VulkanWindowFactory factory);
bool HasVulkanWindowFactory();
void RegisterBuiltinWindowFactories();

} // namespace frame::internal
