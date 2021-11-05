#pragma once

#include <memory>
#include <optional>
#include "Frame/LevelInterface.h"
#include "Frame/Proto/Proto.h"

namespace frame::proto {

	std::optional<std::unique_ptr<MaterialInterface>> ParseMaterialOpenGL(
		const frame::proto::Material& proto_material,
		LevelInterface* level);

} // End namespace frame::proto.
