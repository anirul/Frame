#pragma once

#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Device.h"
#include "../ShaderGLLib/Fill.h"

class Draw : public sgl::DrawInterface
{
public:
	Draw(const std::shared_ptr<sgl::Device>& device) :
		device_(device) {}

public:
	void Startup(const std::pair<std::uint32_t, std::uint32_t> size) override;
	const std::shared_ptr<sgl::Texture>& GetDrawTexture() const override;
	void RunDraw(const double dt) override;
	void Delete() override;

public:
	void SetValue(const int val) override { value_ = val; }

protected:
	sgl::LightManager CreateLightManager() const;

private:
	std::shared_ptr<sgl::Device> device_ = nullptr;
	std::vector<std::shared_ptr<sgl::Texture>> textures_ = {};
	std::shared_ptr<sgl::EffectInterface> clear_ = nullptr;
	std::shared_ptr<sgl::EffectInterface> brightness_ = nullptr;
	std::shared_ptr<sgl::EffectInterface> blur_ = nullptr;
	std::shared_ptr<sgl::EffectInterface> gaussian_blur_horizontal_ = nullptr;
	std::shared_ptr<sgl::EffectInterface> gaussian_blur_vertical_ = nullptr;
	std::shared_ptr<sgl::EffectInterface> addition_ = nullptr;
	std::shared_ptr<sgl::EffectInterface> multiply_ = nullptr;
	int value_ = 0;
};