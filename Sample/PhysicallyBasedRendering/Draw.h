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

protected:
	// Create a light manager filled up with lights.
	std::shared_ptr<sgl::LightManager> CreateLightManager() const;
	// Take 4 textures in and output a texture out.
	std::shared_ptr<sgl::Texture> ComputeLighting(
		const std::vector<std::shared_ptr<sgl::Texture>>& in_textures) const;
	// Take 2 textures in and return a texture out.
	std::shared_ptr<sgl::Texture> AddBloom(
		const std::shared_ptr<sgl::Texture>& texture) const;
	// Create brightness mask.
	std::shared_ptr<sgl::Texture> Draw::CreateBrightness(
		const std::shared_ptr<sgl::Texture>& texture) const;
	// Blur a texture in and return it out.
	std::shared_ptr<sgl::Texture> CreateGaussianBlur(
		const std::shared_ptr<sgl::Texture>& texture) const;
	// Merge many (up to 16) texture into a single one.
	std::shared_ptr<sgl::Texture> Combine(
		const std::vector<std::shared_ptr<sgl::Texture>>& add_textures) const;
	// Move from HDR to screen also apply gamma.
	std::shared_ptr<sgl::Texture> HighDynamicRange(
		const std::shared_ptr<sgl::Texture>& texture,
		const float exposure = 1.0f,
		const float gamma = 2.2f) const;
	// Mesh handling.
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
	std::vector<std::shared_ptr<sgl::Texture>> deferred_textures_ = {};
	std::vector<std::shared_ptr<sgl::Texture>> lighting_textures_ = {};
	std::shared_ptr<sgl::Texture> final_texture_ = nullptr;
	std::shared_ptr<sgl::Device> device_ = nullptr;
	std::shared_ptr<sgl::Program> pbr_program_ = nullptr;
	std::shared_ptr<sgl::Program> lighting_program_ = nullptr;
	std::shared_ptr<sgl::LightManager> light_manager_ = nullptr;
	const sgl::PixelElementSize pixel_element_size_ = 
		sgl::PixelElementSize::HALF;
	const types::draw_model_enum draw_model_ = types::draw_model_enum::MONKEY;
	const types::texture_model_enum texture_model_ = 
		types::texture_model_enum::METAL;
};