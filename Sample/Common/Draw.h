#pragma once

#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Device.h"

class Draw : public sgl::DrawInterface
{
public:
	Draw(const std::shared_ptr<sgl::Device> device) : device_(device) {}

public:
	void Startup(const std::pair<std::uint32_t, std::uint32_t> size) override;
	void RunDraw(const double dt) override;

private:
	sgl::Error& error_ = sgl::Error::GetInstance();
	sgl::Logger& logger_ = sgl::Logger::GetInstance();
	std::shared_ptr<sgl::Device> device_ = nullptr;
};