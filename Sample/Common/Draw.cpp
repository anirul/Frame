#include "Draw.h"
#include <fstream>
#include "../FrameProto/Proto.h"
#include "Name.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	// Load proto from files.
	auto proto_level = 
		frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
			Name::GetGlobalPath() + Name::GetLevelFileName());
	auto proto_texture_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::TextureFile>(
			Name::GetGlobalPath() + proto_level.texture_file());
	auto proto_program_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::ProgramFile>(
			Name::GetGlobalPath() + proto_level.program_file());
	auto proto_scene_tree_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::SceneTreeFile>(
			Name::GetGlobalPath() + proto_level.scene_tree_file());
	auto proto_material_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::MaterialFile>(
			Name::GetGlobalPath() + proto_level.material_file());

	// Load level from proto files.
	auto level = frame::proto::LoadLevelFromProto(
		proto_level, 
		proto_program_file, 
		proto_scene_tree_file, 
		proto_texture_file,
		proto_material_file);
	
	device_->Startup(level);
}

void Draw::RunDraw(const double dt) {}
