#pragma once

#include <memory>
#include "Frame/Proto/Proto.h"
#include "Frame/LevelInterface.h"

namespace frame::proto {

	void ParseSceneTreeFileOpenGL(
		const frame::proto::SceneTreeFile& proto_scene_tree_file,
		const std::shared_ptr<LevelInterface> level);

} // End namespace frame::proto.
