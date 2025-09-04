#include "frame/file/file_system.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string_view>
#include <format>

namespace
{

// This is an awful hack.
constexpr std::array<std::string_view, 2> avoid_elements = {"build", "Build"};

const std::filesystem::path FindElement(
    const std::filesystem::path& file,
    std::function<bool(const std::filesystem::path&)> test)
{
    // Find the current path to the executing path.
    std::filesystem::path path = std::filesystem::current_path();
    auto normalized_input = std::filesystem::absolute(file).lexically_normal();
    if (test(normalized_input))
    {
        // If the user passed an absolute path, honor it even if it contains
        // one of the avoided directory names (like "build").  The avoidance
        // logic is primarily meant for relative asset paths searched from the
        // source tree, not for explicitly selected absolute files.
        if (file.is_absolute())
            return normalized_input;

        bool found = false;
        for (const auto& element : avoid_elements)
        {
            if (normalized_input.string().find(element) != std::string::npos)
            {
                found = true;
            }
        }
        if (!found)
        {
            return normalized_input;
        }
    }

    for (auto i : {0, 1, 2, 3, 4, 5, 6})
    {
        auto new_path = path;
        for (auto j = 0; j < i; ++j)
            new_path /= "../";
        new_path /= file;
        // Resolve the full path before testing it.  Windows in
        // particular struggles with relative paths that contain
        // ".." segments, so normalize and make the path absolute
        // prior to the existence check.
        auto normalized_path =
            std::filesystem::absolute(new_path).lexically_normal();
        if (test(normalized_path))
        {
            // Prune the path from relative elements and keep the
            // resolved absolute version.
            new_path = normalized_path;
            // Search for build (it create a bunch of asset and other
            // element that will confuse the search for the file and path).
            bool found = false;
            for (const auto& element : avoid_elements)
            {
                if (new_path.string().find(element) != std::string::npos)
                {
                    found = true;
                }
            }
            if (!found)
            {
                return new_path;
            }
        }
    }
    throw std::runtime_error(
        std::format("Could not find any element: [{}].", file.string()));
}

} // End namespace.

namespace frame::file
{

std::filesystem::path FindDirectory(std::filesystem::path file)
{
    return FindElement(file, [](const std::filesystem::path& p) {
        return std::filesystem::is_directory(p);
    });
}

std::filesystem::path FindFile(
    std::filesystem::path file,
    const std::vector<std::filesystem::path>& paths)
{
    for (const auto& path : paths)
    {
        auto full_path = path / file;
        if (std::filesystem::is_regular_file(full_path))
        {
            return full_path;
        }
    }
    return FindFile(file);
}

std::filesystem::path FindFile(std::filesystem::path file)
{
    return FindElement(file, [](const std::filesystem::path& p) {
        return std::filesystem::is_regular_file(p);
    });
}

std::string PurifyFilePath(std::filesystem::path file)
{
    auto base = std::filesystem::current_path();
    auto relative = std::filesystem::relative(file, base);
    std::string selection = relative.generic_string();
    if (selection.empty() || selection.front() != '/')
    {
        selection.insert(selection.begin(), '/');
    }
    // if somehow there's a leading slash, drop it:
    if (!selection.empty() && (selection[0] == '/' || selection[0] == '\\'))
    {
        selection.erase(0, 1);
    }
    return selection;
}

} // End namespace frame::file.
