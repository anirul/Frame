#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace sgl {

	class Setting
	{
	public:
		Setting(const std::string& file);
		std::string GetValue(const std::string& name) const;

	public:
		using json = nlohmann::json;
		json content_;
	};

} // End namespace sgl.
