#pragma once

namespace sgl {

	class BindLockInterface 
	{
	public:
		virtual void Bind(const unsigned int slot = 0) const = 0;
		virtual void UnBind() const = 0;
		virtual void LockedBind() const = 0;
		virtual void UnlockedBind() const = 0;
	};

	class ScopedBind
	{
	public:
		ScopedBind(
			const BindLockInterface& bind_locked,
			const unsigned int slot = 0);
		virtual ~ScopedBind();

	private:
		const BindLockInterface& bind_locked_;
	};

} // End namespace sgl.