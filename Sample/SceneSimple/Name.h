#pragma once

#include <string>

// This is a class to hold global parameters.
struct Name {
	static const std::string GetGlobalPath();
	static const std::string GetLevelFileName();
};
