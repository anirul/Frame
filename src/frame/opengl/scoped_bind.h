#pragma once

#include <string>
#include <utility>

#include "frame/opengl/bind_interface.h"

namespace frame::opengl {

/**
 * @class ScopedBind
 * @brief This is a simple kind of scoped lock similar technique to lock underlying class as needed
 * by RAII.
 */
class ScopedBind {
   public:
    /**
     * @brief Constructor this take a class and a slot and lock it for the lifespan of current
     * object in the RAII fashion.
     * @param bind_locked: object on which to lock the bind interface.
     * @param slot: In case needed you can lock by slot.
     */
    ScopedBind(const BindInterface& bind_locked, const unsigned int slot = 0);
    //! @brief free the bind interface.
    virtual ~ScopedBind();

   private:
    const BindInterface& bind_locked_;
};

}  // End namespace frame::opengl.
