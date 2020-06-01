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
	std::shared_ptr<sgl::Mesh> CreateAppleMesh(
		const std::shared_ptr<sgl::Device>& device) const;
	std::shared_ptr<sgl::Mesh> CreateCubeMapMesh(
		const std::shared_ptr<sgl::Device>& device,
		const std::shared_ptr<sgl::TextureCubeMap>& texture) const;

private:
	std::vector<std::shared_ptr<sgl::Texture>> out_textures_ = {};
	std::shared_ptr<sgl::Device> device_ = nullptr;
};
