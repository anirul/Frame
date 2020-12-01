#pragma once

#include "../ShaderGLLib/Device.h"
#include "../ShaderGLLib/Fill.h"
#include "../ShaderGLLib/Logger.h"
#include "../ShaderGLLib/Window.h"

class Draw : public sgl::DrawInterface
{
public:
	Draw(const std::shared_ptr<sgl::Device> device) :
		device_(device) {}

public:
	void Startup(const std::pair<std::uint32_t, std::uint32_t> size) override;
	const std::shared_ptr<sgl::Texture> GetDrawTexture() const override;
	void RunDraw(const double dt) override;

private:
	sgl::Error& error_ = sgl::Error::GetInstance();
	sgl::Logger& logger_ = sgl::Logger::GetInstance();
	std::shared_ptr<sgl::Device> device_ = nullptr;
	std::string preferred_texture_ = "";
	std::map<std::string, std::shared_ptr<sgl::Texture>> texture_map_ = {};
	std::map<std::string, std::shared_ptr<sgl::Effect>> effect_map_ = {};
};
