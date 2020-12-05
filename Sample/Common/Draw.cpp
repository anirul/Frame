#include "Draw.h"
#include <fstream>
#include "../FrameProto/Proto.h"
#include "Name.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	// Load proto from files.
	auto proto_level = 
		LoadProtoFromJsonFile<frame::proto::Level>(
			Name::GetGlobalPath() + Name::GetLevelFileName());
	auto proto_texture_file =
		LoadProtoFromJsonFile<frame::proto::TextureFile>(
			Name::GetGlobalPath() + proto_level.texture_file());
	auto proto_effect_file =
		LoadProtoFromJsonFile<frame::proto::EffectFile>(
			Name::GetGlobalPath() + proto_level.effect_file());
	auto proto_scene_file =
		LoadProtoFromJsonFile<frame::proto::SceneFile>(
			Name::GetGlobalPath() + proto_level.scene_file());
	device_->Startup(
		proto_level, 
		proto_effect_file, 
		proto_scene_file, 
		proto_texture_file);
}

void Draw::RunDraw(const double dt) {}

const std::shared_ptr<sgl::Texture> Draw::GetDrawTexture() const
{
	return texture_map_.at(out_texture_);
}
