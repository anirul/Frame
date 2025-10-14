#pragma once

#include <memory>

#include "frame/json/proto.h"
#include "frame/level_interface.h"
#include "frame/program_interface.h"

namespace frame::vulkan::json
{

std::unique_ptr<frame::ProgramInterface> ParseProgram(
    const frame::proto::Program& proto_program,
    frame::LevelInterface& level);

} // namespace frame::vulkan::json
