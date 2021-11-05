#pragma once

#include <memory>
#include <optional>
#include "Frame/Proto/Proto.h"
#include "Frame/LevelInterface.h"
#include "Frame/ProgramInterface.h"

namespace frame::proto {

	std::optional<std::unique_ptr<ProgramInterface>> 
		ParseProgramOpenGL(
			const frame::proto::Program& proto_program,
			const LevelInterface* level);

} // End namespace frame::proto.
