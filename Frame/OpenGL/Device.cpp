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
		// Process the level to have a direct rendering.
		for (const auto& program_id : 
			GetProgramIdTextureId(level_->GetDefaultOutputTextureId()))
		{
			AddToRenderProgram(program_id);
		}
		// Setup camera.
		SetupCamera();
		rendering_ = std::make_unique<Rendering>(level_, size_);
	}

	void Device::Cleanup()
	{
		level_ = nullptr;
	}

	void Device::Display(const double dt)
	{
		dt_ = dt;

		glClearColor(.2f, 0.f, .2f, 1.0f);
		error_.Display(__FILE__, __LINE__ - 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		SetupCamera();
		rendering_->SetProjection(projection_);
		rendering_->SetView(view_);
		// TODO(anirul): This is suppose to be from mesh.
		rendering_->SetModel(model_);
		for (const auto& program_id : program_render_)
		{
			auto program = level_->GetProgramMap().at(program_id);
#ifdef _DEBUG
			logger_->info(
				"Rendering program {}[{}].", 
				level_->GetNameFromId(program_id), 
				program_id);
#endif // _DEBUG
			SetDepthTest(program->GetDepthTest());
			auto scene_root = program->GetSceneRoot();
			const auto& static_mesh_map = level_->GetStaticMeshMap();
			const auto& it = static_mesh_map.find(scene_root);
			if (it != static_mesh_map.end())
			{
				// This is a special mesh no root node needed, this can be use
				// per example when you have a cube map as a sky box.
				rendering_->RenderMesh(
					this, 
					program.get(), 
					it->second.get(), 
					dt_);
			}
			else
			{
				// Found not a mesh but a hierarchy of meshes, this is the
				// scene rendering part.
				const auto& node_map = level_->GetSceneNodeMap();
				const auto& root_name = level_->GetNameFromId(scene_root);
				for (const auto& node : node_map)
				{
					if (!HasNameInParents(node.first, root_name))
						continue;
					if (!node.second->GetLocalMesh())
						continue;
					rendering_->SetModel(node.second->GetLocalModel(dt_));
					rendering_->RenderMesh(
						this,
						program.get(),
						node.second->GetLocalMesh().get(),
						dt_);
				}
			}
		}
		rendering_->Display(this);
	}

	bool Device::HasNameInParents(EntityId node_id, const std::string& name) const
	{
		const auto& node_map = level_->GetSceneNodeMap();
		const auto& node_interface = node_map.at(node_id);
		const auto& parent_name = node_interface->GetParentName();
		if (parent_name == name) 
		{
			return true;
		}
		else
		{
			const auto& id = level_->GetIdFromName(parent_name);
			return HasNameInParents(id, name);
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

	std::vector<EntityId> Device::GetProgramIdTextureId(
		EntityId texture_id) const
	{
		std::vector<EntityId> program_ids = {};
		// Go through all programs.
		for (const auto& id_program : level_->GetProgramMap())
		{
			for (const auto& output_texture_id : 
				id_program.second->GetOutputTextureIds())
			{
				if (texture_id == output_texture_id)
				{
					program_ids.push_back(id_program.first);
				}
			}
		}
		// Check found anything.
		if (program_ids.empty())
		{
			throw std::runtime_error(
				"no program id that output texture: " + 
				level_->GetNameFromId(texture_id) +
				"[" + std::to_string(texture_id) + "].");
		}
		return program_ids;
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

	void Device::SetDepthTest(bool enable)
	{
		if (enable)
		{
			glEnable(GL_DEPTH_TEST);
			error_.Display(__FILE__, __LINE__ - 1);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
			error_.Display(__FILE__, __LINE__ - 1);
		}
	}

} // End namespace frame::opengl.
