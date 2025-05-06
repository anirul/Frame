#pragma once

#include "frame/json/proto.h"
#include "frame/level_interface.h"
#include "frame/program_interface.h"

namespace frame::proto
{

/**
 * @brief Serialize a program to its proto form.
 * @param program_interface: The program to be serialize.
 * @param level_interface: Interface to the current level.
 * @return The proto that represent the program serialized.
 */
proto::Program SerializeProgram(
    const frame::ProgramInterface& program_interface,
    const frame::LevelInterface& level_interface);

} // namespace frame::proto
