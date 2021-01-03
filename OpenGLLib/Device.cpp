#include "Device.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <random>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include "FrameBuffer.h"
#include "RenderBuffer.h"
#include "Fill.h"
#include "Rendering.h"

namespace frame::opengl {

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

		// Create a frame buffer and a render buffer.
		frame_ = std::make_shared<FrameBuffer>();
		render_ = std::make_shared<RenderBuffer>();
	}

	void Device::Startup(const std::shared_ptr<frame::LevelInterface> level)
	{
		level_ = level;

		SetupCamera();

		cube_ = CreateCubeStaticMesh();
		quad_ = CreateQuadStaticMesh();
	}

	void Device::Cleanup()
	{
		level_ = nullptr;
	}

	void Device::Display(const double dt)
	{
		dt_ = dt;
		std::vector<std::shared_ptr<Effect>> mesh_effects;
		for (const auto& name_effect : effect_map_)
		{
			logger_->info("Display {} effect.", name_effect.first);
			if (name_effect.second->GetRenderInputType() == 
				frame::proto::Effect::SCENE)
			{ 
				logger_->info(
					"\tDisplay {} effect is a mesh effect",
					name_effect.first);
				mesh_effects.push_back(name_effect.second);
			}
			else if (name_effect.second->GetRenderInputType() == 
				frame::proto::Effect::TEXTURE_2D)
			{
				RenderingTexture(
					perspective_,
					view_,
					model_,
					name_effect.second);
			}
			else if (name_effect.second->GetRenderInputType() ==
				frame::proto::Effect::TEXTURE_3D)
			{
				RenderingTextureCubeMap(
					perspective_,
					view_,
					model_,
					name_effect.second);
			}
			else
			{
				throw std::runtime_error(
					"rendering " + 
					std::to_string(name_effect.second->GetRenderInputType()) + 
					" not implemented yet.");
			}
		}
		// Go through the scene and render all meshes.
		for (const auto& name_scene_interface : scene_tree_->GetSceneMap())
		{
			logger_->info(
				"Display {} is being drawn.", 
				name_scene_interface.first);
			if (name_scene_interface.second->GetLocalMesh())
			{
				for (const auto& mesh_effect : mesh_effects)
				{
					RenderingStaticMesh(
						perspective_,
						view_,
						name_scene_interface.second->GetLocalModel(dt_),
						name_scene_interface.second->GetLocalMesh(),
						mesh_effect);
				}
			}
		}
		// Finally display it to the screen.
		static auto program = CreateProgram("Display");
		static auto quad = CreateQuadStaticMesh();
		auto material = std::make_shared<Material>();
		material->AddTexture("Display", texture_map_.at(out_texture_name_));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quad->SetMaterial(material);
		quad->Draw(program);
	}

	void Device::SetupCamera()
	{
		const auto& camera = scene_tree_->GetDefaultCamera();
		perspective_ = camera.ComputeProjection(size_);
		view_ = camera.ComputeView();
	}

	void Device::LoadSceneFromObjFile(const std::string& obj_file)
	{
		if (obj_file.empty())
			throw std::runtime_error("Error invalid file name: " + obj_file);
		std::string mtl_file = "";
		std::string mtl_path = obj_file;
		while (mtl_path.back() != '/' && mtl_path.back() != '\\')
		{
			mtl_path.pop_back();
		}
		std::ifstream obj_ifs(obj_file);
		if (!obj_ifs.is_open())
			throw std::runtime_error("Could not open file: " + obj_file);
		std::string obj_content = "";
		while (!obj_ifs.eof())
		{
			std::string line = "";
			if (!std::getline(obj_ifs, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
				throw std::runtime_error("Error parsing file: " + obj_file);
			if (dump[0] == '#') continue;
			if (dump == "mtllib")
			{
				if (!(iss >> mtl_file))
					throw std::runtime_error("Error parsing file: " + obj_file);
				mtl_file = mtl_path + mtl_file;
				continue;
			}
			obj_content += line + "\n";
		}
		std::ifstream mtl_ifs(mtl_file);
		if (!mtl_ifs.is_open())
			throw std::runtime_error("Error cannot open file: " + mtl_file);
		scene_tree_ = LoadSceneFromObjStream(
			std::istringstream(obj_content),
			obj_file);
		material_map_ = LoadMaterialFromMtlStream(mtl_ifs, mtl_file);
	}

} // End namespace frame::opengl.
