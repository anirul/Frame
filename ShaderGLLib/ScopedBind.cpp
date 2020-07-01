#include "ScopedBind.h"

namespace sgl {

	ScopedBind::ScopedBind(
		const BindLockInterface& bind_locked,
		const unsigned int slot /*= 0*/) :
			bind_locked_(bind_locked)
	{
		bind_locked_.Bind(slot);
		bind_locked_.LockedBind();
	}

	ScopedBind::~ScopedBind()
	{
		bind_locked_.UnlockedBind();
		bind_locked_.UnBind();
	}

} // End namespace sgl.
