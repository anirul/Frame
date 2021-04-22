#pragma once

#include <string>

namespace frame::file {

	// Find the path to a file, this will avoid build directory.
	const std::string FindFile(const std::string& file);
	// Find the path to a directory, this will avoid build directory.
	const std::string FindDirectory(const std::string& file);
	// Split a file name into directory and file, this should use string_view.
	const std::pair<std::string, std::string> SplitFileDirectory(
		const std::string& file);
	// Check if a file exist.
	bool IsFileExist(const std::string& file);
	// Check if a directory exist.
	bool IsDirectoryExist(const std::string& file);

} // End namespace frame::file.
