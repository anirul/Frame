#pragma once

#include <string>
#include <utility>

namespace frame {

/**
 * @class BindInterface
 * @brief Interface for the bind/unbind common feature in OpenGL.
 */
struct BindInterface {
    /**
     * @brief Bind the underlying interface to the current context.
     * @param slot: in case needed (see textures) the slot can be specified.
     */
    virtual void Bind(const unsigned int slot = 0) const = 0;
    //! @brief Unbind free the resource from the current context.
    virtual void UnBind() const = 0;
    //! @brief same as Bind but used by the auto lock system see scoped bind class.
    virtual void LockedBind() const = 0;
    //! @brief same as UnBind but used by the auto lock system see scoped bind class.
    virtual void UnlockedBind() const = 0;
    /**
     * @brief return the id of the current object.
     * @return the id of the current object.
     */
    virtual unsigned int GetId() const = 0;
};

}  // End namespace frame.
