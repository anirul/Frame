#pragma once

#include <memory>
#include "Frame/Proto/Proto.h"
#include "Frame/LevelInterface.h"

namespace frame::proto {

	[[nodiscard]] bool ParseSceneTreeFile(
		const SceneTree& proto_scene_tree,
		LevelInterface* level);

} // End namespace frame::proto.
