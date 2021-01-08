#include "Proto.h"
#include "Frame/LevelBase.h"
#include "Frame/SceneTree.h"
#include "Frame/Proto/ParseLevel.h"

namespace frame::proto {

	std::shared_ptr<LevelInterface> LoadLevelFromProto(
		const std::pair<std::int32_t, std::int32_t> size,
		const Level& proto_level, 
		const ProgramFile& proto_program_file, 
		const SceneTreeFile& proto_scene_tree_file, 
		const TextureFile& proto_texture_file, 
		const MaterialFile& proto_material_file)
	{
		return proto::ParseLevelOpenGL(
			size,
			proto_level, 
			proto_program_file,
			proto_scene_tree_file,
			proto_texture_file,
			proto_material_file);
	}

} // End namespace frame::proto.
