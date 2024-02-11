#pragma once

#include <memory>
#include <mutex>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include <string>

namespace frame
{

/**
 * @enum LoggerType
 *
 * This enum is used to define the type of logger that will be used in the
 * logger class.
 */
enum class LoggerType : std::uint8_t
{
    BUFFER = 1 << 0,
    CONSOLE = 1 << 1,
    FILE = 1 << 2,
};

/**
 * @brief The operator | is used to combine the logger type.
 * @param a The first logger type.
 * @param b The second logger type.
 * @return The combined logger type.
 */
inline LoggerType operator|(LoggerType a, LoggerType b)
{
    return static_cast<LoggerType>(
        static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

/**
 * @brief The operator & is used to combine the logger type.
 * @param a The first logger type.
 * @param b The second logger type.
 * @return The combined logger type.
 */
inline LoggerType operator&(LoggerType a, LoggerType b)
{
    return static_cast<LoggerType>(
        static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

/**
 * @class Logger
 * @brief This is where the speed log logging system is wrapped. This should
 *        be move to the other logging system used in the rest of the
 *        software in the foreseeable future.
 */
class Logger
{
  private:
    /**
     * @brief Private constructor this is a singleton class so no view on
     *        the constructor is allowed.
     */
    Logger(LoggerType logger_type);

  public:
    //! @brief Virtual destructor.
    virtual ~Logger();

  public:
    /**
     * @brief The get instance get a single and non instantiating instance
     *        of the logger function (see singleton).
     * @return A reference to the internal instance of the logger.
     */
    static Logger& GetInstance();
    /**
     * @brief A surcharged operator arrow that allow you to access the
     *        logger functionality.
     * @return A shared pointer to the logger class.
     */
    const std::shared_ptr<spdlog::logger> operator->() const;
    /**
     * @brief The get logs function is used to get the logs that have been
     * added.
	 * @return A vector of strings that contains the logs.
     */
    const std::vector<std::string>& GetLogs() const;
	/// @brief Clear the logs.
    void ClearLogs();

  private:
    std::shared_ptr<spdlog::logger> logger_ptr_ = nullptr;
    std::shared_ptr<spdlog::sinks::base_sink<std::mutex>> gui_logger_sink_ =
        nullptr;
};

} // End namespace frame.
