#include "frame/file/file_system.h"

#include <fmt/core.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string_view>

// Removed the warning from getenv.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable : 4996)
#endif

namespace {

// This is an awful hack.
constexpr std::array<std::string_view, 2> avoid_elements = { "build", "Build" };

const std::filesystem::path FindElement(const std::filesystem::path& file,
                                        std::function<bool(const std::filesystem::path&)> test) {
    // This is a bad hack as this won't prevent people from adding Asset
    // high in the path of the game.
    for (auto i : { 0, 1, 2, 3, 4, 5, 6 }) {
        // Cheat the GITHUB issues.
        const char* cf = std::getenv("GITHUB_WORKSPACE");
        std::string f  = (cf) ? std::string(cf) : std::string{};
        if (!f.empty()) f += "/lvv_gpu/";
        for (auto j = 0; j < i; ++j) f += "../";
        f += file.string();
        if (test(f)) {
            std::filesystem::path p(f);
            std::filesystem::path final_path = std::filesystem::canonical(p);
            bool found                       = false;
            for (const auto& element : avoid_elements) {
                if (final_path.string().find(element) != std::string::npos) found = true;
            }
            if (!found) return final_path;
        }
    }
    throw std::runtime_error(fmt::format("Could not find any element: [{}].", file.string()));
}

}  // End namespace.

namespace frame::file {

const std::filesystem::path FindDirectory(const std::filesystem::path& file) {
    return FindElement(
        file, [](const std::filesystem::path& p) { return std::filesystem::is_directory(p); });
}

const std::filesystem::path FindFile(const std::filesystem::path& file) {
    return FindElement(
        file, [](const std::filesystem::path& p) { return std::filesystem::is_regular_file(p); });
}

}  // End namespace frame::file.
