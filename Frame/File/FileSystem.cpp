#include "FileSystem.h"
#include <filesystem>

namespace frame::file {

	const std::string FindPath(const std::string& file)
	{
		// This is a bad hack as this won't prevent people from adding Asset
		// high in the path of the game.
		for (auto i : {0, 4, 3, 2, 1})
		{
			std::string f;
			for (auto j = 0; j < i; ++j)
				f += "../";
			f += file;
			if (IsDirectoryExist(f))
			{
				std::filesystem::path p(f);
				return std::filesystem::absolute(p).string();
			}
		}
		return "";
	}

	bool IsFileExist(const std::string& file)
	{
		return std::filesystem::is_regular_file(file);
	}

	bool IsDirectoryExist(const std::string& file)
	{
		return std::filesystem::is_directory(file);
	}

} // End namespace frame::file.
