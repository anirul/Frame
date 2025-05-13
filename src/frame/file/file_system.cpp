#include "frame/file/file_system.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string_view>

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
    for (auto i : {0, 1, 2, 3, 4, 5, 6})
    {
        auto new_path = path;
        for (auto j = 0; j < i; ++j)
            new_path /= "../";
        new_path /= file;
        if (test(new_path))
        {
            // Prune the path from relative elements.
            new_path = new_path.lexically_normal();
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

const std::filesystem::path FindDirectory(const std::filesystem::path& file)
{
    return FindElement(file, [](const std::filesystem::path& p) {
        return std::filesystem::is_directory(p);
    });
}

const std::filesystem::path FindFile(const std::filesystem::path& file)
{
    return FindElement(file, [](const std::filesystem::path& p) {
        return std::filesystem::is_regular_file(p);
    });
}

} // End namespace frame::file.
