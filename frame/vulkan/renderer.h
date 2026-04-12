#pragma once

#include <cstddef>

namespace frame::vulkan
{

class Device;

/**
 * @class Renderer
 * @brief Backend-local frame renderer for Vulkan acquire/submit/present work.
 */
class Renderer
{
  public:
    explicit Renderer(Device& device);

    void Display(double dt);
    void Reset();

  private:
    Device& device_;
    std::size_t current_frame_ = 0;
};

} // namespace frame::vulkan
