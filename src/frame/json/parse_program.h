#pragma once

#include <memory>
#include <optional>

#include "frame/json/proto.h"
#include "frame/level_interface.h"
#include "frame/program_interface.h"

namespace frame::proto {

/**
 * @brief Parse a program as an OpenGL object.
 * @param proto_program: The proto form of the program.
 * @param level: A pointer to a level.
 * @return A unique pointer to a program interface or error.
 */
std::optional<std::unique_ptr<ProgramInterface>> ParseProgramOpenGL(
    const frame::proto::Program& proto_program, const LevelInterface* level);

}  // End namespace frame::proto.
