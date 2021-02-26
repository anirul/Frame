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
	}

	Device::~Device() 
	{
		Cleanup();
	}

	void Device::Startup(const std::shared_ptr<frame::LevelInterface> level)
	{
		// Copy level into the local area.
		level_ = level;
		// Preprocess the level to have a direct rendering.
		AddToRenderProgram(
			GetProgramIdTextureId(level_->GetDefaultOutputTextureId()));
		// Setup camera.
		SetupCamera();
		rendering_ = std::make_unique<Rendering>();
	}

	void Device::Cleanup()
	{
		level_ = nullptr;
	}

	void Device::Display(const double dt)
	{
		dt_ = dt;
		rendering_->SetProjection(projection_);
		rendering_->SetView(view_);
		rendering_->SetModel(model_);
		for (const auto& program_id : program_render_)
		{
			auto program = level_->GetProgramMap().at(program_id);
			throw std::runtime_error("Not implemented yet!");
		}
	}

	void Device::SetupCamera()
	{
		auto id = level_->GetDefaultCameraId();
		const auto scene_camera = std::dynamic_pointer_cast<NodeCamera>(
			level_->GetSceneNodeMap().at(id));
		const auto camera = scene_camera->GetCamera();
		projection_ = camera->ComputeProjection(size_);
		view_ = camera->ComputeView();
	}

	EntityId Device::GetProgramIdTextureId(EntityId texture_id) const
	{
		EntityId program_id = 0;
		// Go through all programs.
		for (const auto& id_program : level_->GetProgramMap())
		{
			for (const auto& output_texture_id : 
				id_program.second->GetOutputTextureIds())
			{
				if (texture_id == output_texture_id)
				{
					if (program_id)
					{
						throw std::runtime_error(
							"Texture : " + level_->GetNameFromId(texture_id)
							+ "[" + std::to_string(texture_id) + "]" +
							" cannot be output of more than one program.");
					}
					program_id = id_program.first;
				}
			}
		}
		// Check found anything.
		if (program_id == 0)
		{
			throw std::runtime_error(
				"no program id that output texture: " + 
				level_->GetNameFromId(texture_id) +
				"[" + std::to_string(texture_id) + "].");
		}
		return program_id;
	}

	void Device::AddToRenderProgram(EntityId program_id)
	{
		std::vector<EntityId> texture_ids = {};
		const auto& program = level_->GetProgramMap().at(program_id);
		texture_ids = program->GetInputTextureIds();
		std::sort(texture_ids.begin(), texture_ids.end());
		std::vector<EntityId> program_ids = {};
		for (const auto& id_program : level_->GetProgramMap())
		{
			std::vector<EntityId> output_texture_ids = {};
			output_texture_ids = id_program.second->GetOutputTextureIds();
			std::sort(output_texture_ids.begin(), output_texture_ids.end());
			std::vector<EntityId> intersection = {};
			std::set_intersection(
				output_texture_ids.cbegin(),
				output_texture_ids.cend(),
				texture_ids.cbegin(),
				texture_ids.cend(),
				std::back_inserter(intersection));
			if (intersection.size() > 0)
			{
				program_ids.push_back(id_program.first);
				std::vector<EntityId> difference = {};
				std::set_difference(
					output_texture_ids.cbegin(),
					output_texture_ids.cend(),
					texture_ids.cbegin(), 
					texture_ids.cend(),
					std::back_inserter(difference));
				texture_ids = difference;
			}
		}
		for (const auto& id : program_ids) 
		{
			AddToRenderProgram(id);
		}
		program_render_.push_back(program_id);
	}

	const glm::vec3 Device::GetCameraPosition() const
	{
		auto id = level_->GetDefaultCameraId();
		auto scene_camera = std::dynamic_pointer_cast<NodeCamera>(
			level_->GetSceneNodeMap().at(id));
		return scene_camera->GetCamera()->GetPosition();
	}

	const glm::vec3 Device::GetCameraFront() const
	{
		auto id = level_->GetDefaultCameraId();
		auto scene_camera = std::dynamic_pointer_cast<NodeCamera>(
			level_->GetSceneNodeMap().at(id));
		return scene_camera->GetCamera()->GetFront();
	}

	const glm::vec3 Device::GetCameraRight() const
	{
		auto id = level_->GetDefaultCameraId();
		auto scene_camera = std::dynamic_pointer_cast<NodeCamera>(
			level_->GetSceneNodeMap().at(id));
		return scene_camera->GetCamera()->GetRight();
	}

	const glm::vec3 Device::GetCameraUp() const
	{
		auto id = level_->GetDefaultCameraId();
		auto scene_camera = std::dynamic_pointer_cast<NodeCamera>(
			level_->GetSceneNodeMap().at(id));
		return scene_camera->GetCamera()->GetUp();
	}

	const std::shared_ptr<frame::CameraInterface> Device::GetCamera() const
	{
		auto id = level_->GetDefaultCameraId();
		auto scene_camera = std::dynamic_pointer_cast<NodeCamera>(
			level_->GetSceneNodeMap().at(id));
		return scene_camera->GetCamera();
	}

} // End namespace frame::opengl.
