#pragma once

namespace sgl {

	class BindLock 
	{
	public:
		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;
		virtual void LockedBind() const = 0;
		virtual void UnlockedBind() const = 0;
	};

	class ScopedBind
	{
	public:
		ScopedBind(const BindLock& bind_locked);
		virtual ~ScopedBind();

	private:
		const BindLock& bind_locked_;
	};

} // End namespace sgl.