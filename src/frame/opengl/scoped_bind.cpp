#include "frame/opengl/scoped_bind.h"

#include <stdexcept>

namespace frame::opengl
{

ScopedBind::ScopedBind(
    const BindInterface &bind_locked, const unsigned int slot /*= 0*/)
    : bind_locked_(bind_locked)
{
    bind_locked_.Bind(slot);
    bind_locked_.LockedBind();
}

ScopedBind::~ScopedBind()
{
    bind_locked_.UnlockedBind();
    bind_locked_.UnBind();
}

} // End namespace frame::opengl.
