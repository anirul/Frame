#include "frame/vulkan/scoped_timer.h"

#include <chrono>
#include <utility>

namespace frame::vulkan
{

ScopedTimer::ScopedTimer(const Logger& logger, std::string label)
    : logger_(logger),
      label_(std::move(label)),
      start_(Clock::now())
{
}

ScopedTimer::~ScopedTimer()
{
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - start_);
    logger_->info("{} took {} ms.", label_, elapsed.count());
}

} // namespace frame::vulkan
