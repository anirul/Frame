#include "ScopedBind.h"

namespace sgl {

	ScopedBind::ScopedBind(const BindLock& bind_locked) : 
		bind_locked_(bind_locked)
	{
		bind_locked_.Bind();
		bind_locked_.LockedBind();
	}

	ScopedBind::~ScopedBind()
	{
		bind_locked_.UnlockedBind();
		bind_locked_.UnBind();
	}

} // End namespace sgl.
