#pragma once

#include <chrono>
#include <string>

#include "frame/logger.h"

namespace frame::vulkan
{

class ScopedTimer
{
  public:
    ScopedTimer(const Logger& logger, std::string label);
    ~ScopedTimer();

  private:
    using Clock = std::chrono::steady_clock;
    const Logger& logger_;
    std::string label_;
    Clock::time_point start_;
};

} // namespace frame::vulkan
