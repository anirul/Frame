#pragma once

#include <memory>
#include "Frame/Proto/Proto.h"
#include "Frame/LevelInterface.h"

namespace frame::proto {

	void ParseSceneTreeFile(
		const frame::proto::SceneTreeFile& proto_scene_tree_file,
		LevelInterface* level);

} // End namespace frame::proto.
