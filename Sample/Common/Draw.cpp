#include "Draw.h"
#include <fstream>
#include "Frame/Proto/ParseLevel.h"
#include "Frame/File/FileSystem.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	// Load proto from files.
	auto proto_level =
		frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(path_);
	auto proto_texture_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::TextureFile>(path_);
	auto proto_program_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::ProgramFile>(path_);
	auto proto_scene_tree_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::SceneTreeFile>(path_);
	auto proto_material_file =
		frame::proto::LoadProtoFromJsonFile<frame::proto::MaterialFile>(path_);

	// Load level from proto files.
	auto maybe_level = frame::proto::ParseLevelOpenGL(
		size_,
		proto_level, 
		proto_program_file, 
		proto_scene_tree_file, 
		proto_texture_file,
		proto_material_file);
	if (!maybe_level)
		throw std::runtime_error("Couldn't load level!");
	
	device_->Startup(std::move(maybe_level.value()));
}

void Draw::RunDraw(const double dt) {}
