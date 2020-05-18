#pragma once

#include <memory>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Texture.h"

class Application
{
public:
	// List of draw model available.
	enum class draw_model_enum {
		SPHERE = 0,
		APPLE = 1,
		CUBE = 2,
		TORUS = 3,
		MONKEY = 4,
	};
	// Used to search from string to draw model.
	const std::map<std::string, draw_model_enum> shape_draw_model_map_ =
	{
		{ "Sphere", draw_model_enum::SPHERE },
		{ "Apple",  draw_model_enum::APPLE },
		{ "Cube",   draw_model_enum::CUBE },
		{ "Torus",  draw_model_enum::TORUS },
		{ "Monkey", draw_model_enum::MONKEY },
	};
	// List of texture model available.
	enum class texture_model_enum {
		METAL = 0,
		APPLE = 1,
		PLANKS = 2,
	};
	// Used to search from string to texture model.
	const std::map<std::string, texture_model_enum> name_texture_model_map_ =
	{
		{ "Metal",	texture_model_enum::METAL },
		{ "Apple",	texture_model_enum::APPLE },
		{ "Planks", texture_model_enum::PLANKS },
	};

public:
	Application(
		const std::shared_ptr<sgl::Window>& window,
		const draw_model_enum draw_model = draw_model_enum::SPHERE,
		const texture_model_enum texture_model = texture_model_enum::METAL);
	bool Startup();
	void Run();

protected:
	std::shared_ptr<sgl::Mesh> CreateCubeMapMesh(
		const std::shared_ptr<sgl::Device>& device,
		const std::shared_ptr<sgl::TextureCubeMap>& texture) const;
	std::shared_ptr<sgl::Mesh> CreatePhysicallyBasedRenderedMesh(
		const std::shared_ptr<sgl::Device>& device,
		const std::shared_ptr<sgl::TextureCubeMap>& texture);
	std::vector<std::string> CreateTextures(
		sgl::TextureManager& texture_manager,
		const std::shared_ptr<sgl::TextureCubeMap>& texture) const;
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
	const std::map<draw_model_enum, std::string> draw_model_shape_map_ = {
		{ draw_model_enum::SPHERE, "Sphere" },
		{ draw_model_enum::APPLE,  "Apple" },
		{ draw_model_enum::CUBE,   "Cube" },
		{ draw_model_enum::TORUS,  "Torus" },
		{ draw_model_enum::MONKEY, "Monkey" },
	};
	const std::map<texture_model_enum, std::string> 
		texture_model_texture_map_ = {
		{ texture_model_enum::METAL,  "Metal" },
		{ texture_model_enum::APPLE,  "Apple" },
		{ texture_model_enum::PLANKS, "Planks" },
	};
	std::shared_ptr<sgl::Window> window_ = nullptr;
	std::shared_ptr<sgl::Program> pbr_program_ = nullptr;
	const draw_model_enum draw_model_ = draw_model_enum::SPHERE;
	const texture_model_enum texture_model_ = texture_model_enum::METAL;
};