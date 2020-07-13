#include "Device.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <random>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include "Frame.h"
#include "Render.h"

namespace sgl {

	namespace 
	{
		constexpr float lerp(float a, float b, float t) noexcept
		{
			return a + t * (b - a);
		}
	}

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
			std::make_shared<Texture>(size, pixel_element_size_));
		// Normal.
		deferred_textures_.emplace_back(
			std::make_shared<Texture>(size, pixel_element_size_));
		// MetalicRoughAO.
		deferred_textures_.emplace_back(
			std::make_shared<Texture>(size, pixel_element_size_));
		// Position.
		deferred_textures_.emplace_back(
			std::make_shared<Texture>(size, pixel_element_size_));

		// ViewPosition.
		view_textures_.emplace_back(
			std::make_shared<Texture>(
				size, 
				sgl::PixelElementSize::HALF));
		view_textures_[0]->SetMagFilter(TextureFilter::NEAREST);
		view_textures_[0]->SetMinFilter(TextureFilter::NEAREST);
		view_textures_[0]->SetWrapS(TextureFilter::CLAMP_TO_EDGE);
		view_textures_[0]->SetWrapT(TextureFilter::CLAMP_TO_EDGE);
		// ViewNormal.
		view_textures_.emplace_back(
			std::make_shared<Texture>(
				size, 
				sgl::PixelElementSize::HALF));
		view_textures_[1]->SetMagFilter(TextureFilter::NEAREST);
		view_textures_[1]->SetMinFilter(TextureFilter::NEAREST);
		
		// Noise and kernel.
		std::default_random_engine generator;
		std::uniform_real_distribution<GLfloat> random_length(-1.0, 1.0);
		std::uniform_real_distribution<GLfloat> random_unit(0.0, 1.0);
		for (unsigned int i = 0; i < 64; ++i)
		{
			glm::vec3 sample(
				random_length(generator), 
				random_length(generator),
				random_unit(generator));
			sample = glm::normalize(sample);
			sample *= random_unit(generator);
			float scale = static_cast<float>(i) / 64.0f;
			// Scale sample s.t. they are more aligned to the center of the
			// kernel
			scale = lerp(0.1f, 1.0f, scale * scale);
			sample *= scale;
			kernel_ssao_vec_.emplace_back(sample);
		}
		std::vector<glm::vec3> ssao_noise = {};
		for (unsigned int i = 0; i < 16; ++i)
		{
			ssao_noise.emplace_back(
				glm::vec3(
					random_length(generator), 
					random_length(generator), 
					0.0f));
		}
		noise_texture_ = std::make_shared<Texture>(
			std::pair{4, 4}, 
			ssao_noise.data(),
			sgl::PixelElementSize::HALF, 
			sgl::PixelStructure::GREY_ALPHA);
		noise_texture_->SetWrapS(TextureFilter::REPEAT);
		noise_texture_->SetWrapT(TextureFilter::REPEAT);

		// First texture (suppose to be the base color albedo).
		lighting_textures_.emplace_back(nullptr);
		// The second is the accumulation color.
		lighting_textures_.emplace_back(
			std::make_shared<Texture>(size, pixel_element_size_));

		// Create programs.
		pbr_program_ = Program::CreateProgram("PhysicallyBasedRendering");
		lighting_program_ = Program::CreateProgram("Lighting");
		ssao_program_ = Program::CreateProgram("ScreenSpaceAmbientOcclusion");
		view_program_ = Program::CreateProgram("ViewPositionNormal");
	}

	void Device::AddEffect(std::shared_ptr<EffectInterface>& effect)
	{
		effects_.push_back(effect);
	}

	void Device::Startup(const float fov /*= 65.0f*/)
	{
		fov_ = fov;
		SetupCamera();
		for (auto& effect : effects_)
		{
			effect->Startup(size_);
		}
	}

	void Device::Draw(const double dt)
	{
		static auto out_texture = std::make_shared<Texture>(
			size_, 
			sgl::PixelElementSize::HALF);
		DrawMultiTextures({ out_texture }, nullptr, dt);
		Display(out_texture);
	}

	void Device::DrawDeferred(
		const std::vector<std::shared_ptr<Texture>>& out_textures,
		const double dt)
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
		DrawMultiTextures(temp_textures, nullptr, dt);
	}

	void Device::DrawView(
		const std::vector<std::shared_ptr<Texture>>& out_textures,
		const double dt)
	{
		std::vector<std::shared_ptr<Texture>> temp_textures;
		if (out_textures.empty())
		{
			temp_textures = view_textures_;
		}
		else
		{
			temp_textures = out_textures;
		}
		view_program_->Use();
		view_program_->UniformInt("inverted_normals", 0);
		DrawMultiTextures(temp_textures, view_program_, dt);
	}

	// TODO(anirul):	change this returning the correct lighting texture 
	//					rather than the addition of both.
	void Device::DrawLighting(
		std::shared_ptr<Texture>& out_texture,
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

		// Rendering pipeline.
		Frame frame{};
		Render render{};
		auto size = temp_textures[0]->GetSize();
		frame.AttachRender(render);
		render.CreateStorage(size);

		// Set the view port for rendering.
		glViewport(0, 0, size.first, size.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame.AttachTexture(*lighting_textures_[1]);
		frame.DrawBuffers(1);

		static auto quad = sgl::CreateQuadMesh(lighting_program_);

		auto material = std::make_shared<sgl::Material>();
		material->AddTexture("Ambient", temp_textures[0]);
		material->AddTexture("Normal", temp_textures[1]);
		material->AddTexture("MetalRoughAO", temp_textures[2]);
		material->AddTexture("Position", temp_textures[3]);
		quad->SetMaterial(material);
		quad->Draw();

		TextureAddition(out_texture, lighting_textures_);
	}

	void Device::DrawScreenSpaceAmbientOcclusion(
		std::shared_ptr<Texture>& out_texture,
		const std::vector<std::shared_ptr<Texture>>& in_textures /*= {}*/)
	{
		std::vector<std::shared_ptr<Texture>> temp_textures;
		if (in_textures.empty())
		{
			temp_textures = view_textures_;
		}
		else
		{
			temp_textures = in_textures;
		}
		Frame frame{};
		Render render{};
		ScopedBind scoped_bind(frame);
		auto size = temp_textures[0]->GetSize();
		frame.AttachRender(render);
		render.CreateStorage(size);

		// Set the view port for rendering.
		glViewport(0, 0, size.first, size.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame.AttachTexture(*out_texture);
		frame.DrawBuffers(1);

		// Set the screen space ambient occlusion uniforms.
		ssao_program_->Use();
		for (unsigned int i = 0; i < 64; ++i)
		{
			ssao_program_->UniformVector3(
				"kernel[" + std::to_string(i) + "]",
				kernel_ssao_vec_[i]);
		}
		ssao_program_->UniformVector2(
			"noise_scale", 
			glm::vec2(
				static_cast<float>(size.first) / 4.0f, 
				static_cast<float>(size.second) / 4.0f));
		// Send the projection matrix.
		ssao_program_->UniformMatrix("projection" , perspective_);

		static auto quad = sgl::CreateQuadMesh(ssao_program_);

		std::shared_ptr<sgl::Material> material = 
			std::make_shared<sgl::Material>();
		material->AddTexture("ViewPosition", temp_textures[0]);
		material->AddTexture("ViewNormal", temp_textures[1]);
		material->AddTexture("Noise", noise_texture_);
		quad->SetMaterial(material);
		quad->Draw();
	}

	void Device::DrawHighDynamicRange(
		std::shared_ptr<sgl::Texture>& out_texture,
		const std::shared_ptr<Texture>& in_texture, 
		const float exposure /*= 1.0f*/, 
		const float gamma /*= 2.2f*/)
	{
		Frame frame{};
		Render render{};
		ScopedBind scoped_bind(frame);
		auto size = in_texture->GetSize();
		frame.AttachRender(render);
		render.CreateStorage(size);

		// Set the view port for rendering.
		glViewport(0, 0, size.first, size.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame.AttachTexture(*out_texture);
		frame.DrawBuffers(1);

		auto material = std::make_shared<Material>();
		material->AddTexture("Display", in_texture);
		auto program = Program::CreateProgram("HighDynamicRange");
		program->UniformFloat("exposure", exposure);
		program->UniformFloat("gamma", gamma);
		auto quad = sgl::CreateQuadMesh(program);
		quad->SetMaterial(material);
		quad->Draw();
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
		if (out_textures.empty())
		{
			throw std::runtime_error("Cannot draw on empty textures.");
		}

		// Setup the camera.
		SetupCamera();

		Frame frame{};
		Render render{};
		ScopedBind scoped_bind(frame);
		frame.AttachRender(render);
		render.CreateStorage(size_);

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
			frame.AttachTexture(*texture, Frame::GetFrameColorAttachment(i));
			++i;
		}
		frame.DrawBuffers(static_cast<std::uint32_t>(out_textures.size()));

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
				std::shared_ptr<Material> mat = std::make_shared<Material>(
					*materials_[material_name] + *material_);
				mesh->SetMaterial(mat);
			}
			else
			{
				// Special case this is suppose to be the environment map.
				auto material = std::make_shared<Material>();
				material->AddTexture(
					"Environment", 
					material_->GetTexture("Environment"));
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

	void Device::ClearView()
	{
		for (auto texture : view_textures_)
		{
			texture->Clear(glm::vec4(0.f, 0.f, 0.f, 1.f));
		}
	}

	void Device::ClearDeferred()
	{
		for (auto texture : deferred_textures_)
		{
			texture->Clear(glm::vec4(0.f, 0.f, 0.f, 1.f));
		}
	}

	void Device::AddEnvironment(const std::string& environment_map)
	{
		// Create the skybox.
		auto texture = std::make_shared<TextureCubeMap>(
			environment_map,
			std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
			sgl::PixelElementSize::HALF);
		auto cubemap_program = Program::CreateProgram("CubeMapDeferred");
		cubemap_program->UniformMatrix("projection", GetProjection());
		auto cube_mesh = CreateCubeMesh(cubemap_program);
		material_ = std::make_shared<Material>();
		material_->AddTexture("Skybox", texture);
		cube_mesh->ClearDepthBuffer(true);
		scene_tree_.AddNode(
			std::make_shared<SceneMesh>(cube_mesh), 
			scene_tree_.GetRoot());
		
		// Add the default texture to the texture manager.
		material_->AddTexture("Environment", texture);

		// Create the Monte-Carlo filter.
		auto monte_carlo_prefilter = std::make_shared<sgl::TextureCubeMap>(
			std::make_pair<std::uint32_t, std::uint32_t>(128, 128),
			sgl::PixelElementSize::FLOAT);
		FillProgramMultiTextureCubeMapMipmap(
			std::vector<std::shared_ptr<sgl::Texture>>{ monte_carlo_prefilter },
			std::map<std::string, std::shared_ptr<Texture>>{ 
				{"Environment", material_->GetTexture("Environment") }},
			Program::CreateProgram("MonteCarloPrefilter"),
			5,
			[](const int mipmap, const std::shared_ptr<sgl::Program>& program)
		{
			float roughness = static_cast<float>(mipmap) / 4.0f;
			program->UniformFloat("roughness", roughness);
		});
		material_->AddTexture(
			"MonteCarloPrefilter", 
			monte_carlo_prefilter);

		// Create the Irradiance cube map texture.
		auto irradiance = std::make_shared<sgl::TextureCubeMap>(
			std::make_pair<std::uint32_t, std::uint32_t>(32, 32),
			pixel_element_size_);
		FillProgramMultiTextureCubeMap(
			std::vector<std::shared_ptr<sgl::Texture>>{ irradiance },
			std::map<std::string, std::shared_ptr<Texture>>{
				{ "Environment", material_->GetTexture("Environment") }},
			Program::CreateProgram("IrradianceCubeMap"));
		material_->AddTexture("Irradiance", irradiance);

		// Create the LUT BRDF.
		auto integrate_brdf = std::make_shared<sgl::Texture>(
			std::make_pair<std::uint32_t, std::uint32_t>(512, 512),
			pixel_element_size_);
		FillProgramMultiTexture(
			std::vector<std::shared_ptr<Texture>>{ integrate_brdf },
			std::map<std::string, std::shared_ptr<Texture>>{
				{ "Environment", material_->GetTexture("Environment") }},
			Program::CreateProgram("IntegrateBRDF"));
		material_->AddTexture("IntegrateBRDF", integrate_brdf);
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

	const std::shared_ptr<Texture>& Device::GetNoiseTexture() const
	{
		return noise_texture_;
	}

} // End namespace sgl.
