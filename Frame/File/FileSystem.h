#pragma once

#include <string>

namespace frame::file {

	const std::string FindPath(const std::string& file);
	bool IsFileExist(const std::string& file);
	bool IsDirectoryExist(const std::string& file);

} // End namespace frame::file.
