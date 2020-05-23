#pragma once

#include "../ShaderGLLib/Window.h"

class Draw : public sgl::DrawInterface
{
public:
	Draw(const std::shared_ptr<sgl::WindowInterface>& window) :
		window_(window) {}

public:
	void Initialize(
		const std::pair<std::uint32_t, std::uint32_t> size) override;
	const std::vector<std::shared_ptr<sgl::Texture>>& GetTextures() override;
	void Run(const double dt) override;
	void Delete() override;

private:
	std::vector<std::shared_ptr<sgl::Texture>> out_textures_ = {};
	std::shared_ptr<sgl::WindowInterface> window_ = nullptr;
};
