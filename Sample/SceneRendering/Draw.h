#pragma once

#include <../ShaderGLLib/Window.h>
#include "Types.h"

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

private:
	std::shared_ptr<Device> device_;
};