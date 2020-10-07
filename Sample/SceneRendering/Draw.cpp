#include "Draw.h"
#include <fstream>
#include "../FrameProto/Proto.h"
// TODO remove this. 
#include "EffectBlur.h"
#include "EffectBrightness.h"
#include "EffectHighDynamicRange.h"
#include "EffectLighting.h"
#include "EffectMath.h"
#include "EffectScreenSpaceAmbientOcclusion.h"

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

	std::pair<std::uint32_t, std::uint32_t> size_2 = 
		{ size.first / 2, size.second / 2 };
	std::pair<std::uint32_t, std::uint32_t> size_4 =
		{ size.first / 4, size.second / 4 };
	// Texture storage.
	textures_ = {
		// 0 - Screen space ambient occlusion.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 1 - SSAO + blur also darker.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 2 - Lighting + ambient.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 3 - Lighting + Bloom.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 4 - Lighting + Bloom + SSAO.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 5 - Lighting + Bloom + SSAO + HDR.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 6 - Brightness.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 7 - Brightness + Gaussian Blur horizontal ( / 2).
		std::make_shared<sgl::Texture>(size_2, sgl::PixelElementSize_HALF()),
		// 8 - Brightness + Gaussian Blur full ( / 2).
		std::make_shared<sgl::Texture>(size_2, sgl::PixelElementSize_HALF()),
		// 9 - Deferred shader - Albedo.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 10 - Deferred shader - Normal.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 11 - Deferred shader - Metallic, roughness, ambient occlusion.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 12 - Deferred shader - Position.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 13 - View shader - Position.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 14 - View shader - Normal.
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 15 - Lighting (only the light part).
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize_HALF()),
		// 16 - Brightness + Gaussian Blur horizontal ( / 8 ).
		std::make_shared<sgl::Texture>(size_4, sgl::PixelElementSize_HALF()),
		// 17 - Brightness + Gaussian Blur vertical ( / 8 ).
		std::make_shared<sgl::Texture>(size_4, sgl::PixelElementSize_HALF())
	};
	// Set the texture filter for the view textures.
	textures_[13]->SetMagFilter(sgl::TextureFilter_NEAREST());
	textures_[13]->SetMinFilter(sgl::TextureFilter_NEAREST());
	textures_[13]->SetWrapS(sgl::TextureFilter_CLAMP_TO_EDGE());
	textures_[13]->SetWrapT(sgl::TextureFilter_CLAMP_TO_EDGE());
	textures_[14]->SetMagFilter(sgl::TextureFilter_NEAREST());
	textures_[14]->SetMinFilter(sgl::TextureFilter_NEAREST());
	// Set the texture filter for the Gaussian blur.
	textures_[7]->SetMagFilter(sgl::TextureFilter_LINEAR());
	textures_[7]->SetMinFilter(sgl::TextureFilter_LINEAR());
	textures_[8]->SetMagFilter(sgl::TextureFilter_LINEAR());
	textures_[8]->SetMinFilter(sgl::TextureFilter_LINEAR());
	textures_[16]->SetMagFilter(sgl::TextureFilter_LINEAR());
	textures_[16]->SetMinFilter(sgl::TextureFilter_LINEAR());
	textures_[17]->SetMagFilter(sgl::TextureFilter_LINEAR());
	textures_[17]->SetMinFilter(sgl::TextureFilter_LINEAR());

	device_->SetLightManager(CreateLightManager());

	// Initialize the Brightness effect.
	brightness_ = std::make_shared<sgl::EffectBrightness>(
		textures_[6],
		textures_[2]);
	device_->AddEffect(brightness_);

	// Initialize the Blur effect.
	blur_ = std::make_shared<sgl::EffectBlur>(
		textures_[1], 
		textures_[0], 
		2.f);
	device_->AddEffect(blur_);

	// Initialize the Gaussian Blur effect ( / 2).
	gaussian_blur_h2_ = std::make_shared<sgl::EffectGaussianBlur>(
		textures_[7],
		textures_[6],
		true);
	device_->AddEffect(gaussian_blur_h2_);
	gaussian_blur_v2_ = std::make_shared<sgl::EffectGaussianBlur>(
		textures_[8],
		textures_[7],
		false);
	device_->AddEffect(gaussian_blur_v2_);

	// Initialize the Gaussian Blur effect ( / 8 ).
	gaussian_blur_h4_ = std::make_shared<sgl::EffectGaussianBlur>(
		textures_[16],
		textures_[8],
		true);
	device_->AddEffect(gaussian_blur_h4_);
	gaussian_blur_v4_ = std::make_shared<sgl::EffectGaussianBlur>(
		textures_[17],
		textures_[16],
		false);
	device_->AddEffect(gaussian_blur_v4_);

	// Initialize the Addition effect.
	addition_ = std::make_shared<sgl::EffectAddition>(
		textures_[3],
		std::vector<std::shared_ptr<sgl::Texture>>{ 
			textures_[2], 
			textures_[17] });
	device_->AddEffect(addition_);

	// Initialize the Multiply effect.
	multiply_ = std::make_shared<sgl::EffectMultiply>(
		textures_[4],
		std::vector<std::shared_ptr<sgl::Texture>>{
			textures_[3],
			textures_[1] });
	device_->AddEffect(multiply_);

	// High dynamic range.
	high_dynamic_range_ = std::make_shared<sgl::EffectHighDynamicRange>(
		textures_[5],
		textures_[4]);
	device_->AddEffect(high_dynamic_range_);

	// Lighting.
	lighting_ = std::make_shared<sgl::EffectLighting>(
		textures_[15],
		std::vector<std::shared_ptr<sgl::Texture>>{ 
			textures_[9], 
			textures_[10], 
			textures_[11], 
			textures_[12] },
		device_->GetLightManager(),
		device_->GetCamera());
	device_->AddEffect(lighting_);
	addition_lighting_ = std::make_shared<sgl::EffectAddition>(
		textures_[2],
		std::vector<std::shared_ptr<sgl::Texture>>{ 
			textures_[15], 
			textures_[9] });
	device_->AddEffect(addition_lighting_);

	// SSAO.
	ssao_ = std::make_shared<sgl::EffectScreenSpaceAmbientOcclusion>(
		textures_[0],
		std::vector<std::shared_ptr<sgl::Texture>>{
			textures_[13],
			textures_[14]},
		device_->GetProjection());
	device_->AddEffect(ssao_);
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