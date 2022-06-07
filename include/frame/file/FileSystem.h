#pragma once

#include <string>

namespace frame::file {

/**
 * @brief Find the path to a file, this will avoid build directory.
 * @param file: Basic search pattern for your file "asset/json/test.json".
 * @return Full path to the file.
 */
const std::string FindFile(const std::string& file);
/**
 * @brief Find the path to a directory, this will avoid build directory.
 * @param file: Basic search pattern for your file "asset/json/".
 * @return Full path to the file.
 */
const std::string FindDirectory(const std::string& file);
/**
 * @brief Split a file name into directory and file, this should use string_view.
 * @param file: Basic search pattern for your file "asset/json/test.json".
 * @return A split of file and directory.
 */
const std::pair<std::string, std::string> SplitFileDirectory(const std::string& file);
/**
 * @brief Check if a file exist.
 * @param file: Basic search pattern for your file "asset/json/test.json".
 * @return True if the file was found false otherwise.
 */
bool IsFileExist(const std::string& file);
/**
 * @brief Check if a directory exist.
 * @param file: Basic search pattern for your file "asset/json/".
 * @retutn True if the directory exist false otherwise.
 */
bool IsDirectoryExist(const std::string& file);

}  // End namespace frame::file.
