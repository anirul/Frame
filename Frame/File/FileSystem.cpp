#include "FileSystem.h"

#include <fmt/core.h>

#include <array>
#include <filesystem>
#include <string_view>

namespace {

// This is an awful hack.
constexpr std::array<std::string_view, 2> avoid_elements = { "build", "Build" };

const std::string AddEndSlash(const std::string& path) {
    std::string directory = path;
    const auto id         = directory.find_last_of("\\/");
    if (id != directory.length() - 1) directory += "/";
    return directory;
}

}  // End namespace.

namespace frame::file {

const std::string FindDirectory(const std::string& file) {
    // Treat the case where the end is not a '/' or '\\'.
    std::string directory = AddEndSlash(file);
    // This is a bad hack as this won't prevent people from adding Asset
    // high in the path of the game.
    for (auto i : { 0, 1, 2, 3, 4, 5, 6 }) {
        std::string f;
        for (auto j = 0; j < i; ++j) f += "../";
        f += directory;
        if (IsDirectoryExist(f)) {
            std::filesystem::path p(f);
            std::string final_path = std::filesystem::canonical(p).string();
            bool found             = false;
            for (const auto& element : avoid_elements) {
                if (final_path.find(element) != std::string::npos) found = true;
            }
            if (!found) return AddEndSlash(final_path);
        }
    }
    throw std::runtime_error(fmt::format("Could not find a directory: [{}].", file));
}

const std::string FindFile(const std::string& file) {
    // This is a bad hack as this won't prevent people from adding Asset
    // high in the path of the game.
    for (auto i : { 0, 1, 2, 3, 4, 5, 6 }) {
        std::string f;
        for (auto j = 0; j < i; ++j) f += "../";
        f += file;
        if (IsFileExist(f)) {
            std::filesystem::path p(f);
            std::string final_path = std::filesystem::canonical(p).string();
            bool found             = false;
            for (const auto& element : avoid_elements) {
                if (final_path.find(element) != std::string::npos) found = true;
            }
            if (!found) return final_path;
        }
    }
    throw std::runtime_error(fmt::format("Could not find a file: [{}].", file));
}

bool IsFileExist(const std::string& file) { return std::filesystem::is_regular_file(file); }

bool IsDirectoryExist(const std::string& file) { return std::filesystem::is_directory(file); }

const std::pair<std::string, std::string> SplitFileDirectory(const std::string& file) {
    const auto id = file.find_last_of("\\/");
    if (id == std::string::npos) {
        throw std::runtime_error(
            fmt::format("[{}] is not valid path it doesn't contains any file "
                        "(or any '/' or '\\').",
                        file));
    }
    return { file.substr(0, id + 1), file.substr(id + 1) };
}

}  // End namespace frame::file.
