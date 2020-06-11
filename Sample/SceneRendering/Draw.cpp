#include "Draw.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	final_texture_ = std::make_shared<sgl::Texture>(size);
	ssao_texture_ = std::make_shared<sgl::Texture>(size);
	device_->SetLightManager(CreateLightManager());
}

const std::shared_ptr<sgl::Texture>& Draw::GetDrawTexture() const
{
//  return device_->GetDeferredTexture(1);
//	return device_->GetViewTexture(0);
//	return device_->GetNoiseTexture();
//	return ssao_texture_;
	return final_texture_;
}

void Draw::RunDraw(const double dt)
{
	device_->DrawDeferred(dt);
	device_->DrawView(dt);
	ssao_texture_ = device_->DrawScreenSpaceAmbientOcclusion();
	ssao_texture_ = sgl::TextureBlur(ssao_texture_, 4.0);
	final_texture_ = device_->DrawLighting();
	final_texture_ = device_->DrawBloom(final_texture_);
	final_texture_ = sgl::TextureMultiply({ final_texture_, ssao_texture_ });
	final_texture_ = device_->DrawHighDynamicRange(final_texture_);
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