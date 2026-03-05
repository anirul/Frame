#pragma once

#include <optional>
#include <string>

#include "frame/json/proto.h"

namespace frame::json
{

enum class ShaderBackend
{
    OpenGL,
    Vulkan
};

struct ProgramShaderFiles
{
    std::string vertex_shader;
    std::string fragment_shader;
    std::string compute_shader;
};

std::string ResolveProgramKey(const frame::proto::Program& proto_program);

std::optional<ProgramShaderFiles> ResolveProgramShaderFiles(
    const frame::proto::Program& proto_program,
    ShaderBackend backend);

bool IsRaytracingProgramKey(const std::string& program_key);
bool IsRaytracingBvhProgramKey(const std::string& program_key);

} // namespace frame::json

