#pragma once

#include <memory>
#include "Frame/LevelInterface.h"
#include "Frame/Proto/Proto.h"

namespace frame::proto {

	std::shared_ptr<MaterialInterface> ParseMaterialOpenGL(
		const frame::proto::Material& proto_material,
		const std::shared_ptr<LevelInterface> level);

} // End namespace frame::proto.
