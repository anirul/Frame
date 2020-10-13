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
	sgl::Error& error_ = sgl::Error::GetInstance();
	std::shared_ptr<sgl::Device> device_ = nullptr;
	std::map<std::string, std::shared_ptr<sgl::Texture>> texture_map_ = {};
};
