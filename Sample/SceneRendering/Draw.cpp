#include "Draw.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	textures_ = {
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF)
	};
	device_->SetLightManager(CreateLightManager());
}

const std::shared_ptr<sgl::Texture>& Draw::GetDrawTexture() const
{
//	return device_->GetDeferredTexture(0);
//	return device_->GetViewTexture(1);
//	return device_->GetNoiseTexture();
//	return device_->GetLightingTexture(0);
//	return ssao_texture_;
	return textures_[5];
}

void Draw::RunDraw(const double dt)
{
	device_->DrawDeferred({}, dt);
	device_->DrawView({}, dt);
	// Store preblured SSAO in texture 0.
	device_->DrawScreenSpaceAmbientOcclusion(textures_[0]);
	// Store SSAO in texture 1.
	sgl::TextureBlur(textures_[1], textures_[0], 4.0);
	// Store lighting in texture 2.
	device_->DrawLighting(textures_[2]);
	// Store Bloom in texture 3.
	device_->DrawBloom(textures_[3], textures_[2]);
	// Multiply Bloom and SSAO in texture 4.
	sgl::TextureMultiply(textures_[4], { textures_[3], textures_[1] });
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