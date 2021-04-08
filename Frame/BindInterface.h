#pragma once

#include <utility>
#include <string>

namespace frame {

	struct BindInterface
	{
		virtual void Bind(const unsigned int slot = 0) const = 0;
		virtual void UnBind() const = 0;
		virtual void LockedBind() const = 0;
		virtual void UnlockedBind() const = 0;
		virtual unsigned int GetId() const = 0;
		virtual const std::pair<bool, std::string> GetError() const
		{
			return { true, "Interface" };
		}
	};

} // End namespace frame.
