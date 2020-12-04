#include "Draw.h"
#include <fstream>
#include "../FrameProto/Proto.h"
#include "Name.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	// Load proto from files.
	auto proto_level = LoadProtoFromJsonFile<frame::proto::Level>(
		Name::GetGlobalPath() + Name::GetLevelFileName());
	auto proto_texture_file = LoadProtoFromJsonFile<frame::proto::TextureFile>(
		Name::GetGlobalPath() + proto_level.texture_file());
	auto proto_effect_file = LoadProtoFromJsonFile<frame::proto::EffectFile>(
		Name::GetGlobalPath() + proto_level.effect_file());
	auto proto_scene_file = LoadProtoFromJsonFile<frame::proto::SceneFile>(
		Name::GetGlobalPath() + proto_level.scene_file());

	// Load scenes from proto.
	frame::proto::SceneTree proto_scene_tree;
	for (const auto& proto : proto_scene_file.scene_trees())
	{
		if (proto.name() == proto_level.default_scene_name())
			proto_scene_tree = proto;
	}
	sgl::SceneTree scene_tree(proto_scene_tree);
	
	// Load textures from proto.
	for (const auto& proto_texture : proto_texture_file.textures())
	{
		auto texture = std::make_shared<sgl::Texture>(proto_texture, size);
		texture_map_.insert({ proto_texture.name(), texture });
	}
	out_texture_ = proto_level.default_texture_name();
	if (texture_map_.find(proto_level.default_texture_name()) == 
		texture_map_.end())
	{ 
		throw std::runtime_error(
			"no default texture is loaded: " + 
			proto_level.default_texture_name());
	}

	// Load effects from proto.
	for (const auto& proto_effect : proto_effect_file.effects())
	{
		auto effect = std::make_shared<sgl::Effect>(proto_effect, texture_map_);
		effect_map_.insert({ proto_effect.name(), effect });
	}
}

void Draw::RunDraw(const double dt)
{
	for (auto pair : effect_map_)
	{
		auto effect = pair.second;
		device_->DrawMultiTextures(
			effect->GetProgram(), 
			{ texture_map_.at(out_texture_) }, 
			dt);
	}
}

const std::shared_ptr<sgl::Texture> Draw::GetDrawTexture() const
{
	return texture_map_.at(out_texture_);
}
