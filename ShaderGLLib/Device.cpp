#include "Device.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <random>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include "Frame.h"
#include "Render.h"
#include "Fill.h"

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

		// This should maintain the culling to none.
		// FIXME(anirul): Change this as to be working!
		glDisable(GL_CULL_FACE);
		error_.Display(__FILE__, __LINE__ - 1);
		// glCullFace(GL_BACK);
		// error_.Display(__FILE__, __LINE__ - 1);
		// glFrontFace(GL_CW);
		// error_.Display(__FILE__, __LINE__ - 1);
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

		// Create programs.
		pbr_program_ = Program::CreateProgram("PhysicallyBasedRendering");
		view_program_ = Program::CreateProgram("ViewPositionNormal");

		// Create a frame buffer and a render buffer.
		frame_ = std::make_shared<Frame>();
		render_ = std::make_shared<Render>();
	}

	void Device::AddEffect(std::shared_ptr<Effect>& effect)
	{
		effects_.push_back(effect);
	}

	void Device::Startup(const float fov /*= 65.0f*/)
	{
		fov_ = fov;
		SetupCamera();
		for (auto& effect : effects_)
		{
			effect->Startup(size_, *this);
		}
	}

	void Device::Draw(const double dt)
	{
		static auto out_texture = std::make_shared<Texture>(
			size_, 
			sgl::PixelElementSize_HALF());
		DrawMultiTextures({ out_texture }, nullptr, dt);
		Display(out_texture);
	}

	void Device::DrawDeferred(
		const std::vector<std::shared_ptr<Texture>>& out_textures,
		const double dt)
	{
		assert(out_textures.size() == 4);
		pbr_program_->Use();
		// This should be changed to update from the proto part.
		pbr_program_->Uniform(
			"camera_position",
			GetCamera().GetPosition());
		DrawMultiTextures(out_textures, pbr_program_, dt);
	}

	void Device::DrawView(
		const std::vector<std::shared_ptr<Texture>>& out_textures,
		const double dt)
	{
		assert(out_textures.size() == 2);
		view_program_->Use();
		view_program_->Uniform("inverted_normals", 0);
		DrawMultiTextures(out_textures, view_program_, dt);
	}

	void Device::Display(const std::shared_ptr<Texture>& texture)
	{
		static auto program = Program::CreateProgram("Display");
		static auto quad = CreateQuadMesh(program);
		auto material = std::make_shared<Material>();
		material->AddTexture("Display", texture);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quad->SetMaterial(material);
		quad->Draw();
	}

	void Device::DrawMultiTextures(
		const std::vector<std::shared_ptr<Texture>>& out_textures,
		const std::shared_ptr<Program> program,
		const double dt)
	{
		assert(!out_textures.empty());

		// Setup the camera.
		SetupCamera();

		ScopedBind scoped_bind(*frame_);
		frame_->AttachRender(*render_);
		render_->CreateStorage(size_);

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
			frame_->AttachTexture(*texture, Frame::GetFrameColorAttachment(i));
			++i;
		}
		frame_->DrawBuffers(static_cast<std::uint32_t>(out_textures.size()));

		for (const std::shared_ptr<Scene>& scene : scene_tree_)
		{
			const std::shared_ptr<Mesh>& mesh = scene->GetLocalMesh();
			if (!mesh) continue;
			if (program)
			{
				if (mesh->IsClearDepthBuffer()) continue;
			}

			auto material_name = mesh->GetMaterialName();
			if (materials_.find(material_name) != materials_.end())
			{
				std::shared_ptr<Material> mat = 
					std::make_shared<Material>(*materials_[material_name]);
				if (environment_material_) *mat += *environment_material_;
				mesh->SetMaterial(mat);
			}
			else if (environment_material_)
			{
				// Special case this is suppose to be the environment map.
				auto material = std::make_shared<Material>();
				material->AddTexture(
					"Environment", 
					environment_material_->GetTexture("Environment"));
				mesh->SetMaterial(material);
			}

			std::shared_ptr<Program> temp_program = nullptr;
			if (program)
			{
				temp_program = mesh->GetProgram();
				mesh->SetProgram(program);
			}

			// Draw the mesh.
			mesh->Draw(
				perspective_,
				view_,
				scene->GetLocalModel(dt));

			if (temp_program)
			{
				mesh->SetProgram(temp_program);
			}
		}
	}

	void Device::AddEnvironment(const std::string& environment_map)
	{
		// Create the skybox.
		auto texture = std::make_shared<TextureCubeMap>(
			environment_map,
			std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
			sgl::PixelElementSize_HALF(),
			sgl::PixelStructure_RGB_ALPHA());
		auto cubemap_program = Program::CreateProgram("CubeMapDeferred");
		cubemap_program->Uniform("projection", GetProjection());
		auto cube_mesh = CreateCubeMesh(cubemap_program);
		environment_material_ = std::make_shared<Material>();
		environment_material_->AddTexture("Skybox", texture);
		cube_mesh->ClearDepthBuffer(true);
		scene_tree_.AddNode(
			std::make_shared<SceneMesh>(cube_mesh), 
			scene_tree_.GetRoot());
		
		// Add the default texture to the texture manager.
		environment_material_->AddTexture("Environment", texture);

		// Create the Monte-Carlo filter.
		auto monte_carlo_prefilter = std::make_shared<sgl::TextureCubeMap>(
			std::make_pair<std::uint32_t, std::uint32_t>(128, 128),
			sgl::PixelElementSize_FLOAT());
		FillProgramMultiTextureCubeMapMipmap(
			std::vector<std::shared_ptr<sgl::Texture>>{ monte_carlo_prefilter },
			std::map<std::string, std::shared_ptr<Texture>>{ 
				{"Environment", environment_material_->GetTexture("Environment") }},
			Program::CreateProgram("MonteCarloPrefilter"),
			5,
			[](const int mipmap, const std::shared_ptr<sgl::Program>& program)
		{
			float roughness = static_cast<float>(mipmap) / 4.0f;
			program->Uniform("roughness", roughness);
		});
		environment_material_->AddTexture(
			"MonteCarloPrefilter", 
			monte_carlo_prefilter);

		// Create the Irradiance cube map texture.
		auto irradiance = std::make_shared<sgl::TextureCubeMap>(
			std::make_pair<std::uint32_t, std::uint32_t>(32, 32),
			pixel_element_size_);
		FillProgramMultiTextureCubeMap(
			std::vector<std::shared_ptr<sgl::Texture>>{ irradiance },
			std::map<std::string, std::shared_ptr<Texture>>
				{{ 
					"Environment", 
					environment_material_->GetTexture("Environment") 
				}},
			Program::CreateProgram("IrradianceCubeMap"));
		environment_material_->AddTexture("Irradiance", irradiance);

		// Create the LUT BRDF.
		auto integrate_brdf = std::make_shared<sgl::Texture>(
			std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
			pixel_element_size_);
		FillProgramMultiTexture(
			std::vector<std::shared_ptr<Texture>>{ integrate_brdf },
			std::map<std::string, std::shared_ptr<Texture>>
				{{ 
					"Environment", 
					environment_material_->GetTexture("Environment") 
				}},
			Program::CreateProgram("IntegrateBRDF"));
		environment_material_->AddTexture("IntegrateBRDF", integrate_brdf);
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

	const std::shared_ptr<Texture>& Device::GetDeferredTexture(
		const int i) const
	{
		return deferred_textures_.at(i);
	}

	const std::shared_ptr<Texture>& Device::GetViewTexture(const int i) const
	{
		return view_textures_.at(i);
	}

	const std::shared_ptr<Texture>& Device::GetLightingTexture(
		const int i) const
	{
		return lighting_textures_.at(i);
	}

} // End namespace sgl.
