#include "frame/json/program_catalog.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>

#include "frame/file/file_system.h"
#include "frame/json/parse_json.h"

namespace frame::json
{

namespace
{

using ProgramTable = std::unordered_map<std::string, ProgramShaderFiles>;

std::string ToLowerAscii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    return value;
}

ProgramTable BuildProgramTable(const frame::proto::ProgramCatalogBackend& backend)
{
    ProgramTable table = {};
    for (const auto& entry : backend.entries())
    {
        const auto key = ToLowerAscii(entry.key());
        if (key.empty())
        {
            continue;
        }
        table[key] = ProgramShaderFiles{
            entry.vertex_shader(),
            entry.fragment_shader(),
            entry.compute_shader()};
    }
    return table;
}

const ProgramTable& GetProgramTable(ShaderBackend backend)
{
    static bool loaded = false;
    static ProgramTable opengl_table = {};
    static ProgramTable vulkan_table = {};
    if (!loaded)
    {
        const auto catalog_path =
            frame::file::FindFile("asset/json/program_catalog.json");
        const auto catalog_proto =
            frame::json::LoadProtoFromJsonFile<frame::proto::ProgramCatalog>(
                catalog_path);
        opengl_table = BuildProgramTable(catalog_proto.opengl());
        vulkan_table = BuildProgramTable(catalog_proto.vulkan());
        loaded = true;
    }
    return backend == ShaderBackend::OpenGL ? opengl_table : vulkan_table;
}

} // namespace

std::string ResolveProgramKey(const frame::proto::Program& proto_program)
{
    if (proto_program.has_pipeline_name() &&
        !proto_program.pipeline_name().empty())
    {
        return ToLowerAscii(proto_program.pipeline_name());
    }
    return ToLowerAscii(proto_program.name());
}

std::optional<ProgramShaderFiles> ResolveProgramShaderFiles(
    const frame::proto::Program& proto_program,
    ShaderBackend backend)
{
    const auto& table = GetProgramTable(backend);
    const std::string key = ResolveProgramKey(proto_program);
    if (auto it = table.find(key); it != table.end())
    {
        return it->second;
    }
    const std::string fallback = ToLowerAscii(proto_program.name());
    if (fallback != key)
    {
        if (auto it = table.find(fallback); it != table.end())
        {
            return it->second;
        }
    }
    return std::nullopt;
}

bool IsRaytracingProgramKey(const std::string& program_key)
{
    const std::string lowered = ToLowerAscii(program_key);
    return lowered.find("raytrace") != std::string::npos ||
           lowered.find("raytracing") != std::string::npos;
}

bool IsRaytracingBvhProgramKey(const std::string& program_key)
{
    const std::string lowered = ToLowerAscii(program_key);
    return IsRaytracingProgramKey(lowered) &&
           lowered.find("bvh") != std::string::npos;
}

} // namespace frame::json
