#pragma once

#include <memory>
#include "Frame/Proto/Proto.h"
#include "Frame/ProgramInterface.h"

namespace frame::proto {

	std::shared_ptr<ProgramInterface> ParseProgramOpenGL(
		const frame::proto::Program& proto_program,
		const std::string& default_path,
		const std::map<std::string, std::uint64_t>& name_id_textures);

} // End namespace frame::proto.
