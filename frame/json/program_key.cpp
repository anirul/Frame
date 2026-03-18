#include "frame/json/program_key.h"

#include <algorithm>
#include <cctype>

namespace frame::json
{

namespace
{

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

bool IsRaytracingProgramKey(const std::string& program_key)
{
    const std::string lowered = ToLowerAscii(program_key);
    return lowered.find("raytrace") != std::string::npos ||
           lowered.find("raytracing") != std::string::npos;
}

} // namespace frame::json
