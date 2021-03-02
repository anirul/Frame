#include "Draw.h"
#include <fstream>
#include "Frame/Proto/ParseLevel.h"
#include "Frame/File/FileSystem.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	// Load proto from files.
	auto proto_level = 
		frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
			name_->GetGlobalPath() + name_->GetLevelFileName());
	auto proto_texture_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::TextureFile>(
			name_->GetGlobalPath() + proto_level.texture_file());
	auto proto_program_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::ProgramFile>(
			name_->GetGlobalPath() + proto_level.program_file());
	auto proto_scene_tree_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::SceneTreeFile>(
			name_->GetGlobalPath() + proto_level.scene_tree_file());
	frame::proto::MaterialFile proto_material_file = {};
	if (!proto_level.material_file().empty())
	{
		frame::proto::LoadProtoFromJsonFile<frame::proto::MaterialFile>(
			name_->GetGlobalPath() + proto_level.material_file());
	}

	// Load level from proto files.
	auto level = frame::proto::ParseLevelOpenGL(
		size_,
		name_->GetGlobalPath(),
		proto_level, 
		proto_program_file, 
		proto_scene_tree_file, 
		proto_texture_file,
		proto_material_file);
	
	device_->Startup(level);
}

void Draw::RunDraw(const double dt) {}
