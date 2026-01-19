#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <shaderc/shaderc.h>

namespace frame::vulkan
{

class ShaderCompiler
{
  public:
    std::vector<std::uint32_t> CompileFile(
        const std::filesystem::path& path,
        shaderc_shader_kind kind) const;
    std::vector<std::uint32_t> CompileSource(
        const std::string& source,
        shaderc_shader_kind kind,
        const std::string& identifier) const;
};

} // namespace frame::vulkan
