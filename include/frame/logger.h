#pragma once

#include <spdlog/spdlog.h>

#include <memory>

namespace frame
{

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
    Logger();

  public:
    //! @brief Virtual destructor.
    virtual ~Logger();

  public:
    /**
     * @brief The get instance get a single and non instantiating instance
     *        of the logger function (see singleton).
     * @return A reference to the internal instance of the logger.
     */
    static Logger &GetInstance();
    /**
     * @brief A surcharged operator arrow that allow you to access the
     *        logger functionality.
     * @return A shared pointer to the logger class.
     */
    const std::shared_ptr<spdlog::logger> operator->() const;

  private:
    std::shared_ptr<spdlog::logger> logger_ptr_ = nullptr;
};

} // End namespace frame.
