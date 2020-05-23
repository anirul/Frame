#pragma once

#include <../ShaderGLLib/Window.h>

class Application;

class Draw : public sgl::DrawInterface
{
public:
	Draw(
		const std::shared_ptr<sgl::WindowInterface>& window,
		const std::shared_ptr<Application>& application) :
		window_(window),
		application_(application) {}

public:
	void Initialize(
		const std::pair<std::uint32_t, std::uint32_t> size) override;
	const std::shared_ptr<sgl::Texture>& GetDrawTexture() const override;
	void RunDraw(const double dt) override;
	void Delete() override;

private:
	std::vector<std::shared_ptr<sgl::Texture>> out_textures_;
	std::shared_ptr<sgl::WindowInterface> window_;
	std::shared_ptr<Application> application_;
};