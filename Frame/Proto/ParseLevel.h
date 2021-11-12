#pragma once

#include "Frame/LevelInterface.h"
#include "Frame/Proto/Proto.h"

namespace frame::proto {

	std::optional<std::unique_ptr<LevelInterface>> ParseLevelOpenGL(
		const std::pair<std::int32_t, std::int32_t> size,
		const proto::Level& proto_level,
		const proto::ProgramFile& proto_program_file,
		const proto::SceneTreeFile& proto_scene_tree_file,
		const proto::TextureFile& proto_texture_file,
		const proto::MaterialFile& proto_material_file);

} // End namespace frame::proto.
