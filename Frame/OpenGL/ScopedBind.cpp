#include "ScopedBind.h"

namespace frame::opengl {

	ScopedBind::ScopedBind(
		const BindInterface& bind_locked,
		const unsigned int slot /*= 0*/) :
			bind_locked_(bind_locked)
	{
		bind_locked_.Bind(slot);
		bind_locked_.LockedBind();
	}

	ScopedBind::~ScopedBind()
	{
#ifdef _DEBUG
		auto ret = bind_locked_.GetError();
		if (!ret.first) error_.CreateError(ret.second, __FILE__, __LINE__ - 1);
#endif
		bind_locked_.UnlockedBind();
		bind_locked_.UnBind();
	}

} // End namespace frame::opengl.
