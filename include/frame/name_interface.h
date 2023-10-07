#pragma once

#include <memory>
#include <string>

namespace frame
{

/**
 * @class NameInterface
 * @brief Name an object by adding an interface to have a name and change
 *        it.
 */
struct NameInterface
{
    //! Virtual destructor.
    virtual ~NameInterface() = default;
    /**
     * @brief Get name.
     * @return Name.
     */
    virtual std::string GetName() const = 0;
    /**
     * @brief Set name.
     * @return Name.
     */
    virtual void SetName(const std::string& name) = 0;
};

} // End namespace frame.
