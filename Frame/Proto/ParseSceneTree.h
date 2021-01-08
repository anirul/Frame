#pragma once

#include <memory>
#include "Frame/Proto/Proto.h"
#include "Frame/SceneNodeInterface.h"

namespace frame::proto {

	std::shared_ptr<SceneTreeInterface> ParseSceneTreeOpenGL(
		const frame::proto::SceneTree& proto_scene_tree);

} // End namespace frame::proto.
