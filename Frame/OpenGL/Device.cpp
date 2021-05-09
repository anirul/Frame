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
#include "Renderer.h"

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
		// Setup camera.
		SetupCamera();
		// Create a renderer.
		renderer_ = std::make_unique<Renderer>(level_.get(), this, size_);
	}

	void Device::Cleanup()
	{
		level_ = nullptr;
		renderer_ = nullptr;
	}

	void Device::Display(const double dt)
	{
		dt_ = dt;

		glClearColor(.2f, 0.f, .2f, 1.0f);
		error_.Display(__FILE__, __LINE__ - 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		SetupCamera();
		renderer_->SetProjection(projection_);
		renderer_->SetView(view_);
		// Let's start rendering from root node.
		renderer_->RenderFromRootNode();
		// Present the result on screen.
		renderer_->Display();
	}

	bool Device::HasNameInParents(EntityId node_id, const std::string& name) const
	{
		const auto& node_map = level_->GetSceneNodeMap();
		const auto& node_interface = node_map.at(node_id);
		const auto& parent_name = node_interface->GetParentName();
		if (parent_name.empty()) 
			return false;
		if (parent_name == name) 
			return true;
		const auto& id = level_->GetIdFromName(parent_name);
		return HasNameInParents(id, name);
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
