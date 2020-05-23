#pragma once

#include <../ShaderGLLib/Window.h>
#include "Types.h"

class Draw : public sgl::DrawInterface
{
public:
	Draw(
		const std::shared_ptr<sgl::Device>& device) :
		device_(device) {}

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
	std::shared_ptr<sgl::Mesh> CreateCubeMapMesh(
		const std::shared_ptr<sgl::Device>& device,
		const std::shared_ptr<sgl::TextureCubeMap>& texture) const;
	std::shared_ptr<sgl::Mesh> CreatePhysicallyBasedRenderedMesh(
		const std::shared_ptr<sgl::Device>& device,
		const std::shared_ptr<sgl::TextureCubeMap>& texture);
	std::vector<std::string> CreateTextures(
		sgl::TextureManager& texture_manager,
		const std::shared_ptr<sgl::TextureCubeMap>& texture) const;

private:
	std::vector<std::shared_ptr<sgl::Texture>> out_textures_ = {};
	std::shared_ptr<sgl::Device> device_ = nullptr;
	std::shared_ptr<sgl::Program> pbr_program_ = nullptr;
	const types::draw_model_enum draw_model_ = types::draw_model_enum::SPHERE;
	const types::texture_model_enum texture_model_ = 
		types::texture_model_enum::METAL;
};