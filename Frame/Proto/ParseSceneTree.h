#pragma once

#include <memory>
#include "Frame/Proto/Proto.h"
#include "Frame/LevelInterface.h"

namespace frame::proto {

	[[nodiscard]] bool ParseSceneTreeFile(
		const SceneTreeFile& proto_scene_tree_file,
		LevelInterface* level);

} // End namespace frame::proto.
