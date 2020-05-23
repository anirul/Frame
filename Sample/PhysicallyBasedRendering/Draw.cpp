#include "Draw.h"
#include "../ShaderGLLib/Frame.h"
#include "../ShaderGLLib/Render.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	// Create Environment cube map texture.
	auto texture = std::make_shared<sgl::TextureCubeMap>(
		"../Asset/CubeMap/Hamarikyu.hdr",
		std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
		sgl::PixelElementSize::HALF);

	auto mesh = CreatePhysicallyBasedRenderedMesh(device_, texture);

	// Create the back cube map.
	auto cube_map_mesh = CreateCubeMapMesh(device_, texture);

	// Pack it into a Scene object.
	sgl::SceneTree scene_tree{};
	{
		auto scene_root = std::make_shared<sgl::SceneMatrix>(glm::mat4(1.0));
		scene_tree.AddNode(scene_root);
		scene_tree.AddNode(
			std::make_shared<sgl::SceneMesh>(cube_map_mesh),
			scene_root);
		auto scene_matrix = std::make_shared<sgl::SceneMatrix>(glm::mat4(1.0));
		scene_tree.AddNode(scene_matrix, scene_root);
		scene_tree.AddNode(
			std::make_shared<sgl::SceneMesh>(mesh),
			scene_matrix);
	}

	device_->SetSceneTree(scene_tree);

	out_textures_.emplace_back(
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF));
	out_textures_.emplace_back(
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF));
	out_textures_.emplace_back(
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF));
}

const std::shared_ptr<sgl::Texture>& Draw::GetDrawTexture() const
{
	return out_textures_[0];
}

void Draw::RunDraw(const double dt)
{
	// Update the camera.
	float dtf = static_cast<float>(dt);
	glm::vec4 position = { 0.f, 0.f, 2.f, 1.f };
	glm::mat4 rot_y(1.0f);
	rot_y = glm::rotate(rot_y, dtf * -.1f, glm::vec3(0.f, 1.f, 0.f));
	sgl::Camera cam(glm::vec3(position * rot_y), { 0.f, 0.f, 0.f });
	device_->SetCamera(cam);
	if (pbr_program_)
	{
		// Don't forget to use before setting any uniform.
		pbr_program_->Use();
		pbr_program_->UniformVector3(
			"camera_position",
			device_->GetCamera().GetPosition());
	}
	device_->DrawMultiTextures(out_textures_, dt);
	out_textures_[0] = AddBloom(out_textures_[0]);
}

void Draw::Delete() {}

std::shared_ptr<sgl::Texture> Draw::AddBloom(
	const std::shared_ptr<sgl::Texture>& texture) const
{
	auto brightness = CreateBrightness(texture);
	auto gaussian_blur = CreateGaussianBlur(brightness);
	auto merge = MergeDisplayAndGaussianBlur(texture, gaussian_blur);
	return merge;
}

std::shared_ptr<sgl::Texture> Draw::CreateBrightness(
	const std::shared_ptr<sgl::Texture>& texture) const
{
	const sgl::Error& error = sgl::Error::GetInstance();
	sgl::Frame frame{};
	sgl::Render render{};
	auto size = texture->GetSize();
	frame.BindAttach(render);
	render.BindStorage(size);

	auto texture_out = std::make_shared<sgl::Texture>(
		size,
		sgl::PixelElementSize::HALF);

	// Set the view port for rendering.
	glViewport(0, 0, size.first, size.second);
	error.Display(__FILE__, __LINE__ - 1);

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	error.Display(__FILE__, __LINE__ - 1);

	frame.BindTexture(*texture_out);

	sgl::TextureManager texture_manager;
	texture_manager.AddTexture("Display", texture);
	auto program = sgl::CreateProgram("Brightness");
	auto quad = sgl::CreateQuadMesh(program);
	quad->SetTextures({ "Display" });
	quad->Draw(texture_manager);

	return texture_out;
}

std::shared_ptr<sgl::Texture> Draw::CreateGaussianBlur(
	const std::shared_ptr<sgl::Texture>& texture) const
{
	const sgl::Error& error = sgl::Error::GetInstance();
	sgl::Frame frame[2];
	sgl::Render render{};
	auto size = texture->GetSize();
	frame[0].BindAttach(render);
	frame[1].BindAttach(render);
	render.BindStorage(size);

	std::shared_ptr<sgl::Texture> texture_out[2] = {
		std::make_shared<sgl::Texture>(
			size,
			sgl::PixelElementSize::HALF),
		std::make_shared<sgl::Texture>(
			size,
			sgl::PixelElementSize::HALF)
	};

	// Set the view port for rendering.
	glViewport(0, 0, size.first, size.second);
	error.Display(__FILE__, __LINE__ - 1);

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	error.Display(__FILE__, __LINE__ - 1);

	frame[0].BindTexture(*texture_out[0]);
	frame[1].BindTexture(*texture_out[1]);

	auto program = sgl::CreateProgram("GaussianBlur");
	auto quad = sgl::CreateQuadMesh(program);

	bool horizontal = true;
	bool first_iteration = true;
	for (int i = 0; i < 10; ++i)
	{
		// Reset the texture manager.
		sgl::TextureManager texture_manager;
		program->UniformInt("horizontal", horizontal);
		frame[horizontal].Bind();
		texture_manager.AddTexture(
			"Image",
			(first_iteration) ? texture : texture_out[!horizontal]);
		quad->SetTextures({ "Image" });
		quad->Draw(texture_manager);
		horizontal = !horizontal;
		if (first_iteration) first_iteration = false;
	}

	return texture_out[!horizontal];
}

std::shared_ptr<sgl::Texture> Draw::MergeDisplayAndGaussianBlur(
	const std::shared_ptr<sgl::Texture>& display, 
	const std::shared_ptr<sgl::Texture>& gaussian_blur, 
	const float exposure /*= 1.0f*/) const
{
	const sgl::Error& error = sgl::Error::GetInstance();
	sgl::Frame frame{};
	sgl::Render render{};
	auto size = display->GetSize();
	frame.BindAttach(render);
	render.BindStorage(size);

	auto texture_out = std::make_shared<sgl::Texture>(
		size,
		sgl::PixelElementSize::HALF,
		sgl::PixelStructure::RGB);

	// Set the view port for rendering.
	glViewport(0, 0, size.first, size.second);
	error.Display(__FILE__, __LINE__ - 1);

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	error.Display(__FILE__, __LINE__ - 1);

	frame.BindTexture(*texture_out);

	sgl::TextureManager texture_manager;
	texture_manager.AddTexture("Display", display);
	texture_manager.AddTexture("GaussianBlur", gaussian_blur);
	auto program = sgl::CreateProgram("Combine");
	program->UniformFloat("exposure", exposure);
	auto quad = sgl::CreateQuadMesh(program);
	quad->SetTextures({ "Display", "GaussianBlur" });
	quad->Draw(texture_manager);

	return texture_out;
}

std::vector<std::string> Draw::CreateTextures(
	sgl::TextureManager& texture_manager,
	const std::shared_ptr<sgl::TextureCubeMap>& texture) const
{
	// Get the name of the texture.
	const std::string& name = 
		types::texture_model_texture_map.at(texture_model_);

	// Add the default texture to the texture manager.
	texture_manager.AddTexture("Environment", texture);

	// Create the Monte-Carlo prefilter.
	auto monte_carlo_prefilter = std::make_shared<sgl::TextureCubeMap>(
		std::make_pair<std::uint32_t, std::uint32_t>(128, 128),
		sgl::PixelElementSize::HALF);
	sgl::FillProgramMultiTextureCubeMapMipmap(
		std::vector<std::shared_ptr<sgl::Texture>>{ monte_carlo_prefilter },
		texture_manager,
		{ "Environment" },
		sgl::CreateProgram("MonteCarloPrefilter"),
		5,
		[](const int mipmap, const std::shared_ptr<sgl::Program>& program)
	{
		float roughness = static_cast<float>(mipmap) / 4.0f;
		program->UniformFloat("roughness", roughness);
	});
	texture_manager.AddTexture("MonteCarloPrefilter", monte_carlo_prefilter);

	// Create the Irradiance cube map texture.
	auto irradiance = std::make_shared<sgl::TextureCubeMap>(
		std::make_pair<std::uint32_t, std::uint32_t>(32, 32),
		sgl::PixelElementSize::HALF);
	sgl::FillProgramMultiTextureCubeMap(
		std::vector<std::shared_ptr<sgl::Texture>>{ irradiance },
		texture_manager,
		{ "Environment" },
		sgl::CreateProgram("IrradianceCubeMap"));
	texture_manager.AddTexture("Irradiance", irradiance);

	// Create the LUT BRDF.
	auto integrate_brdf = std::make_shared<sgl::Texture>(
		std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
		sgl::PixelElementSize::HALF);
	sgl::FillProgramMultiTexture(
		std::vector<std::shared_ptr<sgl::Texture>>{ integrate_brdf },
		texture_manager,
		{},
		sgl::CreateProgram("IntegrateBRDF"));
	texture_manager.AddTexture("IntegrateBRDF", integrate_brdf);

	// Create the texture and bind it to the mesh.
	texture_manager.AddTexture(
		"Color",
		std::make_shared<sgl::Texture>("../Asset/" + name + "/Color.jpg"));
	texture_manager.AddTexture(
		"Normal",
		std::make_shared<sgl::Texture>("../Asset/" + name + "/Normal.jpg"));
	texture_manager.AddTexture(
		"Metallic",
		std::make_shared<sgl::Texture>("../Asset/" + name + "/Metalness.jpg"));
	texture_manager.AddTexture(
		"Roughness",
		std::make_shared<sgl::Texture>("../Asset/" + name + "/Roughness.jpg"));
	texture_manager.AddTexture(
		"AmbientOcclusion",
		std::make_shared<sgl::Texture>(
			"../Asset/" + name + "/AmbientOcclusion.jpg"));

	return
	{
		"Color",
		"Normal",
		"Metallic",
		"Roughness",
		"AmbientOcclusion",
		"MonteCarloPrefilter",
		"Irradiance",
		"IntegrateBRDF"
	};
}

std::shared_ptr<sgl::Mesh> Draw::CreatePhysicallyBasedRenderedMesh(
	const std::shared_ptr<sgl::Device>& device,
	const std::shared_ptr<sgl::TextureCubeMap>& texture)
{
	// Create the physically based rendering program.
	pbr_program_ = sgl::CreateProgram("PhysicallyBasedRendering");
	pbr_program_->UniformMatrix("projection", device->GetProjection());
	pbr_program_->UniformMatrix("view", device->GetView());
	pbr_program_->UniformMatrix("model", device->GetModel());

	// Create lights.
	sgl::LightManager light_manager{};
	const float light_value = 300.f;
	const glm::vec3 light_vec(light_value, light_value, light_value);
	light_manager.AddLight(sgl::Light({ 10.f, 10.f, 10.f }, light_vec));
	light_manager.AddLight(sgl::Light({ 10.f, -10.f, 10.f }, light_vec));
	light_manager.AddLight(sgl::Light({ -10.f, 10.f, 10.f }, light_vec));
	light_manager.AddLight(sgl::Light({ -10.f, -10.f, 10.f }, light_vec));
	light_manager.RegisterToProgram(pbr_program_);
	device->SetLightManager(light_manager);

	// Mesh creation.
	auto mesh = std::make_shared<sgl::Mesh>(
		"../Asset/Model/" + 
		types::draw_model_shape_map.at(draw_model_) + 
		".obj",
		pbr_program_);

	// Get the texture manager.
	auto texture_manager = device->GetTextureManager();

	// Set the texture to be used in the shader.
	mesh->SetTextures(CreateTextures(texture_manager, texture));
	device->SetTextureManager(texture_manager);

	return mesh;
}

std::shared_ptr<sgl::Mesh> Draw::CreateCubeMapMesh(
	const std::shared_ptr<sgl::Device>& device,
	const std::shared_ptr<sgl::TextureCubeMap>& texture) const
{
	// Create the cube map program.
	auto cubemap_program = sgl::CreateProgram("CubeMapHighDynamicRange");
	cubemap_program->UniformMatrix("projection", device->GetProjection());

	// Create the mesh for the cube.
	auto cube_mesh = sgl::CreateCubeMesh(cubemap_program);

	// Get the texture manager.
	auto texture_manager = device->GetTextureManager();
	texture_manager.AddTexture("Skybox", texture);
	cube_mesh->SetTextures({ "Skybox" });
	device->SetTextureManager(texture_manager);

	// Enable the cleaning of the depth.
	cube_mesh->ClearDepthBuffer(true);
	return cube_mesh;
}