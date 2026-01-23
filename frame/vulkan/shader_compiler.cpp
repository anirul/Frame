#include "frame/vulkan/shader_compiler.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>
#include <system_error>

#include <shaderc/shaderc.hpp>

namespace frame::vulkan
{

std::vector<std::uint32_t> ShaderCompiler::CompileFile(
    const std::filesystem::path& path,
    shaderc_shader_kind kind) const
{
    auto resolve_spv_path = [](const std::filesystem::path& source_path) {
        const auto parent = source_path.parent_path();
        if (!parent.empty() && parent.filename() == "vulkan")
        {
            const auto asset_root = parent.parent_path().parent_path();
            return asset_root / "cache" / "shader" / "vulkan" /
                (source_path.filename().string() + ".spv");
        }
        return std::filesystem::path(source_path.string() + ".spv");
    };

    const std::filesystem::path spv_path = resolve_spv_path(path);
    const std::filesystem::path legacy_spv_path = path.string() + ".spv";
    std::error_code source_error;
    const bool source_exists =
        std::filesystem::exists(path, source_error) && !source_error;
    std::optional<std::filesystem::file_time_type> source_time;
    if (source_exists)
    {
        auto time = std::filesystem::last_write_time(path, source_error);
        if (!source_error)
        {
            source_time = time;
        }
    }

    auto try_load_spv = [&](const std::filesystem::path& candidate)
        -> std::optional<std::vector<std::uint32_t>> {
        std::error_code cache_error;
        if (!std::filesystem::exists(candidate, cache_error) || cache_error)
        {
            return std::nullopt;
        }
        if (source_time)
        {
            const auto cache_time =
                std::filesystem::last_write_time(candidate, cache_error);
            if (!cache_error && cache_time < *source_time)
            {
                return std::nullopt;
            }
        }
        std::ifstream cache_file(candidate, std::ios::binary);
        if (!cache_file)
        {
            return std::nullopt;
        }
        cache_file.seekg(0, std::ios::end);
        const std::streamsize cache_size = cache_file.tellg();
        cache_file.seekg(0, std::ios::beg);
        if (cache_size > 0 && (cache_size % 4) == 0)
        {
            std::vector<std::uint32_t> cached(
                static_cast<std::size_t>(cache_size / 4));
            if (cache_file.read(
                    reinterpret_cast<char*>(cached.data()),
                    cache_size))
            {
                return cached;
            }
        }
        return std::nullopt;
    };

    if (auto cached = try_load_spv(spv_path))
    {
        return *cached;
    }
    if (spv_path != legacy_spv_path)
    {
        if (auto cached = try_load_spv(legacy_spv_path))
        {
            return *cached;
        }
    }

    if (!source_exists)
    {
        throw std::runtime_error(std::format(
            "Unable to open shader file {} (no SPV cache found)",
            path.string()));
    }

    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error(std::format(
            "Unable to open shader file {}", path.string()));
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    const std::string shader_source = stream.str();

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto result = compiler.CompileGlslToSpv(
        shader_source,
        kind,
        path.string().c_str(),
        "main",
        options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error(result.GetErrorMessage());
    }

    std::vector<std::uint32_t> compiled{result.cbegin(), result.cend()};
    std::error_code write_error;
    std::filesystem::create_directories(
        spv_path.parent_path(), write_error);
    std::ofstream cache_out(spv_path, std::ios::binary | std::ios::trunc);
    if (cache_out)
    {
        cache_out.write(
            reinterpret_cast<const char*>(compiled.data()),
            static_cast<std::streamsize>(
                compiled.size() * sizeof(std::uint32_t)));
    }
    return compiled;
}

std::vector<std::uint32_t> ShaderCompiler::CompileSource(
    const std::string& source,
    shaderc_shader_kind kind,
    const std::string& identifier) const
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    auto result = compiler.CompileGlslToSpv(
        source,
        kind,
        identifier.c_str(),
        "main",
        options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error(result.GetErrorMessage());
    }
    return {result.cbegin(), result.cend()};
}

} // namespace frame::vulkan
