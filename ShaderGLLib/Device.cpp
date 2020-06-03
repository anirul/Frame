#include "Device.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include "Frame.h"
#include "Render.h"

namespace sgl {

	Device::Device(
		void* gl_context, 
		const std::pair<std::uint32_t, std::uint32_t> size) : 
		gl_context_(gl_context),
		size_(size)
	{
		// Initialize GLEW.
		if (GLEW_OK != glewInit())
		{
			throw std::runtime_error("couldn't initialize GLEW");
		}

		// Enable blending to 1 - source alpha.
		glEnable(GL_BLEND);
		error_.Display(__FILE__, __LINE__ - 1);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		error_.Display(__FILE__, __LINE__ - 1);
		// Enable Z buffer.
		glEnable(GL_DEPTH_TEST);
		error_.Display(__FILE__, __LINE__ - 1);
		glDepthFunc(GL_LEQUAL);
		error_.Display(__FILE__, __LINE__ - 1);
		// Enable seamless cube map.
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		error_.Display(__FILE__, __LINE__ - 1);

		// Albedo.
		deferred_textures_.emplace_back(
			std::make_shared<sgl::Texture>(size, pixel_element_size_));
		// Normal.
		deferred_textures_.emplace_back(
			std::make_shared<sgl::Texture>(size, pixel_element_size_));
		// MetalicRoughAO.
		deferred_textures_.emplace_back(
			std::make_shared<sgl::Texture>(size, pixel_element_size_));
		// Position.
		deferred_textures_.emplace_back(
			std::make_shared<sgl::Texture>(size, pixel_element_size_));

		// First texture (suppose to be the base color albedo).
		lighting_textures_.emplace_back(nullptr);
		// The second is the accumulation color.
		lighting_textures_.emplace_back(
			std::make_shared<sgl::Texture>(size, pixel_element_size_));

		// Create programs.
		pbr_program_ = CreateProgram("PhysicallyBasedRendering");
		lighting_program_ = CreateProgram("Lighting");
	}

	void Device::Startup(const float fov /*= 65.0f*/)
	{
		fov_ = fov;
		SetupCamera();
	}

	void Device::Draw(const double dt)
	{
		Display(DrawTexture(dt));
	}

	void Device::DrawDeferred(
		const double dt, 
		const std::vector<std::shared_ptr<Texture>>& out_textures /*= {}*/)
	{
		std::vector<std::shared_ptr<Texture>> temp_textures;
		if (out_textures.empty())
		{
			temp_textures = deferred_textures_;
		}
		else
		{
			temp_textures = out_textures;
		}
		pbr_program_->Use();
		pbr_program_->UniformVector3(
			"camera_position",
			GetCamera().GetPosition());
		DrawMultiTextures(temp_textures, dt);
	}

	std::shared_ptr<sgl::Texture> Device::DrawLighting(
		const std::vector<std::shared_ptr<Texture>>& in_textures /*= {}*/)
	{
		std::vector<std::shared_ptr<Texture>> temp_textures;
		if (in_textures.empty())
		{
			temp_textures = deferred_textures_;
		}
		else
		{
			temp_textures = in_textures;
		}
		// Make the PBR deferred lighting step.
		lighting_textures_[0] = temp_textures[0];
		lighting_program_->Use();
		lighting_program_->UniformVector3(
			"camera_position",
			GetCamera().GetPosition());
		light_manager_.RegisterToProgram(lighting_program_);
		sgl::Frame frame{};
		sgl::Render render{};
		auto size = temp_textures[0]->GetSize();
		frame.BindAttach(render);
		render.BindStorage(size);

		// Set the view port for rendering.
		glViewport(0, 0, size.first, size.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame.BindTexture(*lighting_textures_[1]);
		frame.DrawBuffers(1);

		auto quad = sgl::CreateQuadMesh(lighting_program_);

		sgl::TextureManager texture_manager{};
		texture_manager.AddTexture("Ambient", temp_textures[0]);
		texture_manager.AddTexture("Normal", temp_textures[1]);
		texture_manager.AddTexture("MetalRoughAO", temp_textures[2]);
		texture_manager.AddTexture("Position", temp_textures[3]);
		quad->SetTextures({ "Ambient", "Normal", "MetalRoughAO", "Position" });
		quad->Draw(texture_manager);

		return TextureCombine(lighting_textures_);
	}

	std::shared_ptr<sgl::Texture> Device::DrawBloom(
		const std::shared_ptr<Texture>& texture)
	{
		auto brightness = TextureBrightness(texture);
		auto gaussian_blur = TextureGaussianBlur(brightness);
		return TextureCombine({ texture, gaussian_blur });
	}

	std::shared_ptr<sgl::Texture> Device::DrawHighDynamicRange(
		const std::shared_ptr<Texture>& texture, 
		const float exposure /*= 1.0f*/, 
		const float gamma /*= 2.2f*/)
	{
		sgl::Frame frame{};
		sgl::Render render{};
		auto size = texture->GetSize();
		frame.BindAttach(render);
		render.BindStorage(size);

		auto texture_out = std::make_shared<sgl::Texture>(
			size,
			pixel_element_size_);

		// Set the view port for rendering.
		glViewport(0, 0, size.first, size.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame.BindTexture(*texture_out);
		frame.DrawBuffers(1);

		sgl::TextureManager texture_manager;
		texture_manager.AddTexture("Display", texture);
		auto program = sgl::CreateProgram("HighDynamicRange");
		program->UniformFloat("exposure", exposure);
		program->UniformFloat("gamma", gamma);
		auto quad = sgl::CreateQuadMesh(program);
		quad->SetTextures({ "Display" });
		quad->Draw(texture_manager);

		return texture_out;
	}

	void Device::Display(const std::shared_ptr<Texture>& texture)
	{
		auto program = CreateProgram("Display");
		auto quad = CreateQuadMesh(program);
		TextureManager texture_manager{};
		texture_manager.AddTexture("Display", texture);
		quad->SetTextures({ "Display" });
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quad->Draw(texture_manager);
	}

	std::shared_ptr<Texture> Device::DrawTexture(const double dt)
	{
		auto texture = std::make_shared<Texture>(
			size_, 
			PixelElementSize::FLOAT, 
			PixelStructure::RGB);
		DrawMultiTextures({ texture }, dt);
		return texture;
	}

	void Device::DrawMultiTextures(
		const std::vector<std::shared_ptr<Texture>>& out_textures, 
		const double dt)
	{
		if (out_textures.empty())
		{
			throw std::runtime_error("Cannot draw on empty textures.");
		}

		// Setup the camera.
		SetupCamera();

		Frame frame{};
		Render render{};
		frame.BindAttach(render);
		render.BindStorage(size_);

		// Set the view port for rendering.
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClearColor(.2f, 0.f, .2f, 1.0f);
		error_.Display(__FILE__, __LINE__ - 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		int i = 0;
		for (const auto& texture : out_textures)
		{
			frame.BindTexture(*texture, Frame::GetFrameColorAttachment(i));
			++i;
		}
		frame.DrawBuffers(static_cast<std::uint32_t>(out_textures.size()));

		for (const std::shared_ptr<Scene>& scene : scene_tree_)
		{
			const std::shared_ptr<Mesh>& mesh = scene->GetLocalMesh();
			if (!mesh)
			{
				continue;
			}

			auto material_name = mesh->GetMaterialName();
			if (materials_.find(material_name) != materials_.end())
			{ 
				auto material = materials_[material_name];
				material->UpdateTextureManager(texture_manager_);
				mesh->SetTextures(material->GetTextures());
			}

			// Draw the mesh.
			mesh->Draw(
				texture_manager_,
				perspective_,
				view_,
				scene->GetLocalModel(dt));
		}
	}

	void Device::AddEnvironment(const std::string& environment_map)
	{
		// Create the skybox.
		auto texture = std::make_shared<TextureCubeMap>(
			environment_map,
			std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
			sgl::PixelElementSize::HALF);
		auto cubemap_program = CreateProgram("CubeMapDeferred");
		cubemap_program->UniformMatrix("projection", GetProjection());
		auto cube_mesh = CreateCubeMesh(cubemap_program);
		texture_manager_.AddTexture("Skybox", texture);
		cube_mesh->SetTextures({ "Skybox" });
		cube_mesh->ClearDepthBuffer(true);
		scene_tree_.AddNode(
			std::make_shared<SceneMesh>(cube_mesh), 
			scene_tree_.GetRoot());
		
		// Add the default texture to the texture manager.
		texture_manager_.AddTexture("Environment", texture);

		// Create the Monte-Carlo prefilter.
		auto monte_carlo_prefilter = std::make_shared<sgl::TextureCubeMap>(
			std::make_pair<std::uint32_t, std::uint32_t>(128, 128),
			sgl::PixelElementSize::FLOAT);
		sgl::FillProgramMultiTextureCubeMapMipmap(
			std::vector<std::shared_ptr<sgl::Texture>>{ monte_carlo_prefilter },
			texture_manager_,
			{ "Environment" },
			sgl::CreateProgram("MonteCarloPrefilter"),
			5,
			[](const int mipmap, const std::shared_ptr<sgl::Program>& program)
		{
			float roughness = static_cast<float>(mipmap) / 4.0f;
			program->UniformFloat("roughness", roughness);
		});
		texture_manager_.AddTexture(
			"MonteCarloPrefilter", 
			monte_carlo_prefilter);

		// Create the Irradiance cube map texture.
		auto irradiance = std::make_shared<sgl::TextureCubeMap>(
			std::make_pair<std::uint32_t, std::uint32_t>(32, 32),
			pixel_element_size_);
		sgl::FillProgramMultiTextureCubeMap(
			std::vector<std::shared_ptr<sgl::Texture>>{ irradiance },
			texture_manager_,
			{ "Environment" },
			sgl::CreateProgram("IrradianceCubeMap"));
		texture_manager_.AddTexture("Irradiance", irradiance);

		// Create the LUT BRDF.
		auto integrate_brdf = std::make_shared<sgl::Texture>(
			std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
			pixel_element_size_);
		sgl::FillProgramMultiTexture(
			std::vector<std::shared_ptr<sgl::Texture>>{ integrate_brdf },
			texture_manager_,
			{},
			sgl::CreateProgram("IntegrateBRDF"));
		texture_manager_.AddTexture("IntegrateBRDF", integrate_brdf);
	}

	void Device::SetupCamera()
	{
		// Set the perspective matrix.
		const float aspect =
			static_cast<float>(size_.first) / static_cast<float>(size_.second);
		perspective_ = glm::perspective(
			glm::radians(fov_),
			aspect,
			0.1f,
			100.0f);

		// Set the camera.
		view_ = camera_.GetLookAt();
	}

	void Device::LoadSceneFromObjFile(const std::string& obj_file)
	{
		if (obj_file.empty())
		{
			throw std::runtime_error(
				"Error invalid file name: " + obj_file);
		}
		std::string mtl_file = "";
		std::string mtl_path = obj_file;
		while (mtl_path.back() != '/' && mtl_path.back() != '\\')
		{
			mtl_path.pop_back();
		}
		std::ifstream obj_ifs(obj_file);
		if (!obj_ifs.is_open())
		{
			throw std::runtime_error(
				"Could not open file: " + obj_file);
		}
		std::string obj_content = "";
		while (!obj_ifs.eof())
		{
			std::string line = "";
			if (!std::getline(obj_ifs, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
			{
				throw std::runtime_error(
					"Error parsing file: " + obj_file);
			}
			if (dump[0] == '#') continue;
			if (dump == "mtllib")
			{
				if (!(iss >> mtl_file))
				{
					throw std::runtime_error(
						"Error parsing file: " + obj_file);
				}
				mtl_file = mtl_path + mtl_file;
				continue;
			}
			obj_content += line + "\n";
		}
		std::ifstream mtl_ifs(mtl_file);
		if (!mtl_ifs.is_open())
		{
			throw std::runtime_error(
				"Error cannot open file: " + mtl_file);
		}
		scene_tree_ = LoadSceneFromObjStream(
			std::istringstream(obj_content),
			pbr_program_,
			obj_file);
		materials_ = LoadMaterialFromMtlStream(mtl_ifs, mtl_file);
	}

} // End namespace sgl.
