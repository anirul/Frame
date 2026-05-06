#pragma once

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "frame/logger.h"

namespace frame::vulkan
{

namespace detail
{

using VulkanProfileClock = std::chrono::steady_clock;

inline std::string GetVulkanProfileEnvValue()
{
#if defined(_WIN32)
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, "FRAME_VULKAN_PROFILE") != 0 ||
        value == nullptr)
    {
        return {};
    }
    std::string result(value);
    std::free(value);
    return result;
#else
    const char* value = std::getenv("FRAME_VULKAN_PROFILE");
    return value ? std::string(value) : std::string{};
#endif
}

inline bool IsVulkanFrameProfilerEnabled()
{
    static const bool enabled = []
    {
        std::string value = GetVulkanProfileEnvValue();
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            });
        return !value.empty() && value != "0" && value != "false" &&
               value != "off";
    }();
    return enabled;
}

struct VulkanProfileSample
{
    double total_ms = 0.0;
    double max_ms = 0.0;
    std::size_t count = 0;
};

} // namespace detail

class VulkanFrameProfiler
{
  public:
    void RecordSample(const char* label, double elapsed_ms)
    {
        if (!detail::IsVulkanFrameProfilerEnabled())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto& sample = samples_[label];
        sample.total_ms += elapsed_ms;
        sample.max_ms = std::max(sample.max_ms, elapsed_ms);
        ++sample.count;
    }

    void RecordCounter(const char* label, std::size_t value)
    {
        if (!detail::IsVulkanFrameProfilerEnabled() || value == 0)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        counters_[label] += value;
    }

    void EndFrame(const frame::Logger& logger)
    {
        if (!detail::IsVulkanFrameProfilerEnabled())
        {
            return;
        }

        std::vector<std::pair<std::string, detail::VulkanProfileSample>>
            samples;
        std::vector<std::pair<std::string, std::size_t>> counters;
        std::size_t frame_count = 0;
        double wall_ms = 0.0;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++frame_count_;
            if (frame_count_ < kLogFrameInterval)
            {
                return;
            }

            frame_count = frame_count_;
            wall_ms = std::chrono::duration_cast<
                          std::chrono::duration<double, std::milli>>(
                          detail::VulkanProfileClock::now() - window_start_)
                          .count();
            samples.assign(samples_.begin(), samples_.end());
            counters.assign(counters_.begin(), counters_.end());
            ResetLocked();
        }

        std::sort(
            samples.begin(),
            samples.end(),
            [](const auto& lhs, const auto& rhs)
            {
                return lhs.second.total_ms > rhs.second.total_ms;
            });
        std::sort(counters.begin(), counters.end());

        const double fps =
            wall_ms > 0.0
                ? static_cast<double>(frame_count) * 1000.0 / wall_ms
                : 0.0;
        logger->warn(
            "Vulkan frame profile avg over {} frames ({:.1f} fps): {}",
            frame_count,
            fps,
            FormatSamples(samples, frame_count));
        if (!counters.empty())
        {
            logger->warn(
                "Vulkan frame profile counters over {} frames: {}",
                frame_count,
                FormatCounters(counters, frame_count));
        }
    }

  private:
    static constexpr std::size_t kLogFrameInterval = 120;

    void ResetLocked()
    {
        samples_.clear();
        counters_.clear();
        frame_count_ = 0;
        window_start_ = detail::VulkanProfileClock::now();
    }

    static std::string FormatSamples(
        const std::vector<std::pair<std::string, detail::VulkanProfileSample>>&
            samples,
        std::size_t frame_count)
    {
        if (samples.empty())
        {
            return "no samples";
        }

        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2);
        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            if (i > 0)
            {
                stream << ", ";
            }
            const auto& [label, sample] = samples[i];
            const double average_per_frame =
                sample.total_ms / static_cast<double>(frame_count);
            const double calls_per_frame =
                static_cast<double>(sample.count) /
                static_cast<double>(frame_count);
            const double average_per_call =
                sample.count > 0
                    ? sample.total_ms / static_cast<double>(sample.count)
                    : 0.0;
            stream << label << "=" << average_per_frame << "ms/frame";
            if (sample.count != frame_count)
            {
                stream << " (" << calls_per_frame << " calls/frame, "
                       << average_per_call << "ms/call)";
            }
            stream << " max=" << sample.max_ms << "ms";
        }
        return stream.str();
    }

    static std::string FormatCounters(
        const std::vector<std::pair<std::string, std::size_t>>& counters,
        std::size_t frame_count)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2);
        for (std::size_t i = 0; i < counters.size(); ++i)
        {
            if (i > 0)
            {
                stream << ", ";
            }
            const auto& [label, value] = counters[i];
            stream << label << "="
                   << static_cast<double>(value) /
                          static_cast<double>(frame_count)
                   << "/frame";
        }
        return stream.str();
    }

  private:
    std::mutex mutex_;
    std::unordered_map<std::string, detail::VulkanProfileSample> samples_;
    std::unordered_map<std::string, std::size_t> counters_;
    std::size_t frame_count_ = 0;
    detail::VulkanProfileClock::time_point window_start_ =
        detail::VulkanProfileClock::now();
};

inline VulkanFrameProfiler& GetVulkanFrameProfiler()
{
    static VulkanFrameProfiler profiler;
    return profiler;
}

class VulkanProfileScope
{
  public:
    explicit VulkanProfileScope(const char* label)
        : enabled_(detail::IsVulkanFrameProfilerEnabled()),
          label_(label),
          start_(detail::VulkanProfileClock::now())
    {
    }

    ~VulkanProfileScope()
    {
        if (!enabled_)
        {
            return;
        }
        const double elapsed_ms =
            std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                detail::VulkanProfileClock::now() - start_)
                .count();
        GetVulkanFrameProfiler().RecordSample(label_, elapsed_ms);
    }

  private:
    bool enabled_ = false;
    const char* label_ = "";
    detail::VulkanProfileClock::time_point start_;
};

class VulkanProfileFrame
{
  public:
    explicit VulkanProfileFrame(const frame::Logger& logger)
        : enabled_(detail::IsVulkanFrameProfilerEnabled()), logger_(logger)
    {
    }

    ~VulkanProfileFrame()
    {
        if (enabled_)
        {
            GetVulkanFrameProfiler().EndFrame(logger_);
        }
    }

  private:
    bool enabled_ = false;
    const frame::Logger& logger_;
};

inline void RecordVulkanProfileCounter(const char* label, std::size_t value = 1)
{
    GetVulkanFrameProfiler().RecordCounter(label, value);
}

} // namespace frame::vulkan
