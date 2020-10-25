#include "Draw.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include "../FrameProto/Proto.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	// Open the Display.json file (in Asset/).
	std::string file_name = "../Asset/SceneRendering.json";
	std::ifstream ifs(file_name.c_str(), std::ios::in);
	if (!ifs) error_.CreateError(
		"Couldn't open file " + file_name, 
		__FILE__, 
		__LINE__ - 4);
	// Create a display (empty for now).
	frame::proto::Display display{};
	// Get the content of the file.
	std::string contents(std::istreambuf_iterator<char>(ifs), {});
	// Set the options for the parsing.
	google::protobuf::util::JsonParseOptions options{};
	options.ignore_unknown_fields = true;
	// Parse the json file.
	auto status = google::protobuf::util::JsonStringToMessage(
		contents,
		&display,
		options);
	if (!status.ok()) error_.CreateError(
		status.ToString(), 
		__FILE__, 
		__LINE__ - 7);
	// TODO(anirul): change these for "error_" checks.
	assert(display.out_textures_size() > 0);
	assert(display.textures_size() > 0);
	assert(display.effects_size() > 0);
	// Create the textures.
	for (const auto& texture : display.textures())
	{
		texture_map_.emplace(
			texture.name(), 
			std::make_shared<sgl::Texture>(texture, size));
#ifdef _DEBUG
		const auto texture_size = texture_map_[texture.name()]->GetSize();
		logger_->info(
			"loading texture: {} ({}-{}).", 
			texture.name(), 
			texture_size.first, 
			texture_size.second);
#endif
	}
	preferred_texture_ = *display.out_textures().begin();
	for (const auto& effect : display.effects())
	{
		effect_map_.emplace(
			effect.name(),
			std::make_shared<sgl::Effect>(effect, texture_map_));
#ifdef _DEBUG
		logger_->info("loading effect: {}.", effect.name());
#endif
	}
	logger_->info("setting preferred texture to: {}", preferred_texture_);
}

const std::shared_ptr<sgl::Texture>& Draw::GetDrawTexture() const
{
	return texture_map_.at(preferred_texture_);
}

void Draw::RunDraw(const double dt)
{
	// Have to be cleaned.
	texture_map_["view_position"]->Clear(glm::vec4(0, 0, 0, 1));
	texture_map_["view_normal"]->Clear(glm::vec4(0, 0, 0, 1));
	// Do the deferred and view computation.
	device_->DrawDeferred(
		{	
			texture_map_["deferred_albedo"], 
			texture_map_["deferred_normal"], 
			texture_map_["deferred_mrao"], 
			texture_map_["deferred_position"] 
		}, 
		dt);
	device_->DrawView(
		{ texture_map_["view_position"], texture_map_["view_normal"] }, 
		dt);
	// 13 14 -> 0 - Compute the Screen space ambient occlusion.
	// ssao_->Draw();
	// 0 -> 1 - Blur the Screen space ambient occlusion.
	// blur_->Draw();

	// Store lighting in texture 2.
	// 9 10 11 12 -> 15
	// lighting_->Draw();
	// 15 + 9 -> 2
	// addition_lighting_->Draw();

	// Store Bloom in texture 3.
	// 2 -> 6
	// brightness_->Draw();
	// 6 -> 7
	// gaussian_blur_h2_->Draw();
	// 7 -> 8
	// gaussian_blur_v2_->Draw();
	// 8 -> 16
	// gaussian_blur_h4_->Draw();
	// 16 -> 17
	// gaussian_blur_v4_->Draw();

	// 2 + 17 -> 3
	// addition_->Draw();
	// 3 -> 4 - Multiply Bloom and SSAO.
	// multiply_->Draw();
	// 4 -> 5 - Get the final texture in texture 5.
	// high_dynamic_range_->Draw();
}

void Draw::Delete() {}
