#pragma once

#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Device.h"

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

protected:
	sgl::LightManager CreateLightManager() const;

private:
	std::shared_ptr<sgl::Device> device_ = nullptr;
	std::shared_ptr<sgl::Texture> final_texture_ = nullptr;
	std::shared_ptr<sgl::Texture> ssao_texture_ = nullptr;
};