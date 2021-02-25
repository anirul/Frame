#pragma once

#include <memory>
#include "Frame/Proto/Proto.h"
#include "Frame/LevelInterface.h"
#include "Frame/ProgramInterface.h"

namespace frame::proto {

	std::shared_ptr<ProgramInterface> ParseProgramOpenGL(
		const frame::proto::Program& proto_program,
		const std::string& default_path,
		const LevelInterface* level);

} // End namespace frame::proto.
