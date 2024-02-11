#include "frame/logger.h"

#include "include/frame/gui/gui_logger_sink.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace frame
{

Logger::Logger(LoggerType logger_type)
{
    std::vector<spdlog::sink_ptr> sinks;
    if (static_cast<bool>(logger_type & LoggerType::CONSOLE))
    {
        sinks.push_back(
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }
    if (static_cast<bool>(logger_type & LoggerType::BUFFER))
    {
        gui_logger_sink_ = std::make_shared<frame::gui::GuiLoggerSink>();
        sinks.push_back(gui_logger_sink_);
    }
    if (static_cast<bool>(logger_type & LoggerType::FILE))
    {
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            "./log.txt", true));
    }
    logger_ptr_ =
        std::make_shared<spdlog::logger>("frame", begin(sinks), end(sinks));
    spdlog::register_logger(logger_ptr_);
    spdlog::set_default_logger(logger_ptr_);
    logger_ptr_->info("start logging!");
}

Logger::~Logger() = default;

Logger& Logger::GetInstance()
{
#ifdef _DEBUG
    static Logger logger_(
        LoggerType::BUFFER | LoggerType::FILE | LoggerType::CONSOLE);
#else
    static Logger logger_(LoggerType::BUFFER | LoggerType::FILE);
#endif
    return logger_;
}

const std::shared_ptr<spdlog::logger> Logger::operator->() const
{
    return logger_ptr_;
}

const std::vector<std::string>& Logger::GetLogs() const
{
    auto gui_logger_sink =
        std::dynamic_pointer_cast<frame::gui::GuiLoggerSink>(gui_logger_sink_);
    if (gui_logger_sink)
    {
        return gui_logger_sink->GetLogs();
    }
    else
    {
        throw std::runtime_error("no buffer sink?");
    }
}

void Logger::ClearLogs()
{
    auto gui_logger_sink =
        std::dynamic_pointer_cast<frame::gui::GuiLoggerSink>(gui_logger_sink_);
    if (gui_logger_sink)
    {
        gui_logger_sink->ClearLogs();
    }
    else
    {
        throw std::runtime_error("no buffer sink?");
    }
}

} // End namespace frame.
