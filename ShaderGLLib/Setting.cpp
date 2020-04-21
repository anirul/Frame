#include "Setting.h"
#include <fstream>

namespace sgl {

	Setting::Setting(const std::string& file)
	{
		std::ifstream ifs(file);
		if (!ifs.is_open())
		{
			throw std::runtime_error("Couldn't open file: " + file);
		}
		ifs >> content_;
	}

	std::string Setting::GetValue(const std::string& name) const
	{
		return content_[name].get<std::string>();
	}

} // End namespace sgl.
