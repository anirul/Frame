#include "Draw.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	textures_ = {
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF)
	};
	device_->SetLightManager(CreateLightManager());
	
	// Initialize the Clear effect.
	clear_ = std::make_shared<sgl::EffectClear>(
		textures_[0],
		glm::vec4{ 1.f });
	device_->AddEffect(clear_);

	// Initialize the Brightness effect.
	brightness_ = std::make_shared<sgl::EffectBrightness>(
		textures_[6],
		textures_[2]);
	device_->AddEffect(brightness_);

	// Initialize the Blur effect.
	blur_ = std::make_shared<sgl::EffectBlur>(
		textures_[1], 
		textures_[0], 
		4.f);
	device_->AddEffect(blur_);

	// Initialize the Gaussian Blur effect.
	gaussian_blur_ = std::make_shared<sgl::EffectGaussianBlur>(
		textures_[7],
		textures_[6]);
	device_->AddEffect(gaussian_blur_);

	// Initialize the Addition effect.
	addition_ = std::make_shared<sgl::EffectAddition>(
		textures_[3],
		std::vector<std::shared_ptr<sgl::Texture>>{ 
			textures_[2], 
			textures_[7] });
	device_->AddEffect(addition_);

	// Initialize the Multiply effect.
	multiply_ = std::make_shared<sgl::EffectMultiply>(
		textures_[4],
		std::vector<std::shared_ptr<sgl::Texture>>{
			textures_[3],
			textures_[1] });
	device_->AddEffect(multiply_);
}

const std::shared_ptr<sgl::Texture>& Draw::GetDrawTexture() const
{
//	return device_->GetDeferredTexture(0);
//	return device_->GetViewTexture(1);
//	return device_->GetNoiseTexture();
//	return device_->GetLightingTexture(0);
//	return ssao_texture_;
	return textures_[0];
}

void Draw::RunDraw(const double dt)
{
	device_->DrawDeferred({}, dt);
	device_->DrawView({}, dt);
	// Clear the texture 0.
	clear_->Draw(dt);
	// Store pre-blured SSAO in texture 0.
	device_->DrawScreenSpaceAmbientOcclusion(textures_[0]);
	// Store lighting in texture 2.
	device_->DrawLighting(textures_[2]);
	// Store SSAO in texture 1.
	blur_->Draw(dt);
	// Store Bloom in texture 3.
#if 1
	// 2 -> 6
	brightness_->Draw(dt);
	// 6 -> 7
	gaussian_blur_->Draw(dt);
	// 2 + 7 -> 3
	addition_->Draw(dt);
#else
	device_->DrawBloom(textures_[3], textures_[2]);
#endif
	// Multiply Bloom and SSAO in texture 4.
#if 1
	multiply_->Draw(dt);
#else
	sgl::TextureMultiply(textures_[4], { textures_[3], textures_[1] });
#endif
	// Get the final texture in texture 5.
	device_->DrawHighDynamicRange(textures_[5], textures_[4]);
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