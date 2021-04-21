#include "FileSystem.h"
#include <array>
#include <filesystem>
#include <string_view>
#include <fmt/core.h>

namespace {

	// This is an awful hack.
	constexpr std::array<std::string_view, 2> avoid_elements = 
	{ 
		"build", 
		"Build" 
	};

} // End namespace.

namespace frame::file {

	const std::string FindDirectory(const std::string& file)
	{
		// This is a bad hack as this won't prevent people from adding Asset
		// high in the path of the game.
		for (auto i : { 0, 1, 2, 3, 4, 5 })
		{
			std::string f;
			for (auto j = 0; j < i; ++j)
				f += "../";
			f += file;
			if (IsDirectoryExist(f))
			{
				std::filesystem::path p(f);
				std::string final_path = std::filesystem::absolute(p).string();
				bool found = false;
				for (const auto& element : avoid_elements)
				{
					if (final_path.find(element) != std::string::npos)
						found = true;
				}
				if (!found) return final_path;
			}
		}
		throw std::runtime_error(
			fmt::format("Could not find a directory: [{}].", file));
	}

	const std::string FindFile(const std::string& file)
	{
		// This is a bad hack as this won't prevent people from adding Asset
		// high in the path of the game.
		for (auto i : { 0, 1, 2, 3, 4, 5 })
		{
			std::string f;
			for (auto j = 0; j < i; ++j)
				f += "../";
			f += file;
			if (IsFileExist(f))
			{
				std::filesystem::path p(f);
				std::string final_path = std::filesystem::absolute(p).string();
				bool found = false;
				for (const auto& element : avoid_elements)
				{
					if (final_path.find(element) != std::string::npos)
						found = true;
				}
				if (!found) return final_path;
			}
		}
		throw std::runtime_error(
			fmt::format("Could not find a file: [{}].", file));
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
