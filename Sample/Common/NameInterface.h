#pragma once

#include <string>

struct NameInterface
{
	virtual const std::string& GetGlobalPath() const = 0;
	virtual const std::string& GetLevelFileName() const = 0;
};
