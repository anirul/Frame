#pragma once

#include <string>

#include "frame/json/proto.h"

namespace frame::json
{

std::string ResolveProgramKey(const frame::proto::Program& proto_program);
bool IsRaytracingProgramKey(const std::string& program_key);

} // namespace frame::json
