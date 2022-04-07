#pragma once

#include <memory>
#include <string>

namespace frame {

	struct NameInterface 
	{
		virtual std::string GetName() const = 0;
		virtual void SetName(const std::string& name) = 0;
		virtual ~NameInterface() = default;
	};

} // End namespace frame.
