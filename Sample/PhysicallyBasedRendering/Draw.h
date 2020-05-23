#pragma once

#include <../ShaderGLLib/Window.h>

class Application;

class Draw : public sgl::DrawInterface
{
public:
	Draw(
		const std::shared_ptr<sgl::Device>& device,
		const std::shared_ptr<sgl::Program>& pbr_program) :
		device_(device),
		pbr_program_(pbr_program) {}

public:
	void Startup(const std::pair<std::uint32_t, std::uint32_t> size) override;
	const std::shared_ptr<sgl::Texture>& GetDrawTexture() const override;
	void RunDraw(const double dt) override;
	void Delete() override;

protected:
	std::shared_ptr<sgl::Texture> AddBloom(
		const std::shared_ptr<sgl::Texture>& texture) const;
	std::shared_ptr<sgl::Texture> CreateBrightness(
		const std::shared_ptr<sgl::Texture>& texture) const;
	std::shared_ptr<sgl::Texture> CreateGaussianBlur(
		const std::shared_ptr<sgl::Texture>& texture) const;
	std::shared_ptr<sgl::Texture> MergeDisplayAndGaussianBlur(
		const std::shared_ptr<sgl::Texture>& display,
		const std::shared_ptr<sgl::Texture>& gaussian_blur,
		const float exposure = 1.0f) const;

private:
	std::vector<std::shared_ptr<sgl::Texture>> out_textures_ = {};
	std::shared_ptr<sgl::Device> device_ = nullptr;
	std::shared_ptr<sgl::Program> pbr_program_ = nullptr;
};