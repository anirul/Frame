#pragma once

#include "Frame/LevelInterface.h"
#include "Frame/Proto/Proto.h"

namespace frame::proto {

	std::optional<std::unique_ptr<LevelInterface>> ParseLevelOpenGL(
		const std::pair<std::int32_t, std::int32_t> size,
		const proto::Level& proto_level);

} // End namespace frame::proto.
