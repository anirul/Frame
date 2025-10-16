#pragma once

#include <filesystem>

namespace frame::file
{

/**
 * @brief Find the path to a file, this will avoid build directory.
 * @param file: Basic search pattern for your file "asset/json/test.json".
 * @return Full path to the file.
 */
std::filesystem::path FindFile(std::filesystem::path file);
/**
 * @brief Find the path to a directory, this will avoid build directory.
 * @param file: Basic search pattern for your file "asset/json/".
 * @return Full path to the file.
 */
std::filesystem::path FindDirectory(std::filesystem::path file);
/**
 * @brief Purify the path name to contain only the relevant part for export.
 * @param file: Basic file name to be purify.
 * @return The purified path file.
 */
std::string PurifyFilePath(std::filesystem::path file);

} // End namespace frame::file.
