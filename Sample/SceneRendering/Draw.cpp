#include "Draw.h"
#include <fstream>
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
	std::for_each(
		display.textures().begin(), 
		display.textures().end(), 
		[this, size](const frame::proto::Texture& texture)
		{
			texture_map_.emplace(
				texture.name(), 
				std::make_shared<sgl::Texture>(texture, size));
		});
#ifdef _DEBUG
	std::for_each(
		texture_map_.begin(),
		texture_map_.end(),
		[this](const auto& name_texture_pair)
		{
			// TODO: Output logs (on screen with IMGUI).
		});
#endif

	device_->SetLightManager(CreateLightManager());
}

const std::shared_ptr<sgl::Texture>& Draw::GetDrawTexture() const
{
	return textures_[value_ % textures_.size()];
}

void Draw::RunDraw(const double dt)
{
	// Have to be cleaned.
	textures_[13]->Clear(glm::vec4(0, 0, 0, 1));
	textures_[14]->Clear(glm::vec4(0, 0, 0, 1));
	// Do the deferred and view computation.
	device_->DrawDeferred(
		{ textures_[9], textures_[10], textures_[11], textures_[12] }, 
		dt);
	device_->DrawView(
		{ textures_[13], textures_[14] }, 
		dt);
	// 13 14 -> 0 - Compute the Screen space ambient occlusion.
	ssao_->Draw();
	// 0 -> 1 - Blur the Screen space ambient occlusion.
	blur_->Draw();

	// Store lighting in texture 2.
	// 9 10 11 12 -> 15
	lighting_->Draw();
	// 15 + 9 -> 2
	addition_lighting_->Draw();

	// Store Bloom in texture 3.
	// 2 -> 6
	brightness_->Draw();
	// 6 -> 7
	gaussian_blur_h2_->Draw();
	// 7 -> 8
	gaussian_blur_v2_->Draw();
	// 8 -> 16
	gaussian_blur_h4_->Draw();
	// 16 -> 17
	gaussian_blur_v4_->Draw();

	// 2 + 17 -> 3
	addition_->Draw();
	// 3 -> 4 - Multiply Bloom and SSAO.
	multiply_->Draw();
	// 4 -> 5 - Get the final texture in texture 5.
	high_dynamic_range_->Draw();
}

void Draw::Delete() {}

sgl::LightManager Draw::CreateLightManager() const
{
	// Create lights.
	sgl::LightManager light_manager = {};
	const float light_value = 100.f;
	const glm::vec3 light_vec(light_value, light_value, light_value);
	light_manager.AddLight(
		std::make_shared<sgl::LightPoint>(
			glm::vec3{ 5.f, 5.f, 5.f },
			light_vec));
	light_manager.AddLight(
		std::make_shared<sgl::LightPoint>(
			glm::vec3{ -5.f, 5.f, 5.f },
			light_vec));
	light_manager.AddLight(
		std::make_shared<sgl::LightPoint>(
			glm::vec3{ 5.f, 5.f, -5.f },
			light_vec));
	light_manager.AddLight(
		std::make_shared<sgl::LightPoint>(
			glm::vec3{ -5.f, 5.f, -5.f },
			light_vec));
	return light_manager;
}
