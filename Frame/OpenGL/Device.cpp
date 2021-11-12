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

	void Device::Startup(std::unique_ptr<frame::LevelInterface>&& level)
	{
		// Copy level into the local area.
		level_ = std::move(level);
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

	bool Device::HasNameInParents(
		EntityId node_id, 
		const std::string& name) const
	{
		auto maybe_id = level_->GetParentId(node_id);
		if (!maybe_id) return false;
		EntityId parent_id = maybe_id.value();
		auto maybe_name = level_->GetNameFromId(parent_id);
		if (!maybe_name) return false;
		if (name == maybe_name.value()) return true;
		return HasNameInParents(parent_id, name);
	}

	void Device::SetupCamera()
	{
		view_ = GetCamera().ComputeView();
	}

	const glm::vec3 Device::GetCameraPosition() const
	{
		return GetCamera().GetPosition();
	}

	const glm::vec3 Device::GetCameraFront() const
	{
		return GetCamera().GetFront();
	}

	const glm::vec3 Device::GetCameraRight() const
	{
		return GetCamera().GetRight();
	}

	const glm::vec3 Device::GetCameraUp() const
	{
		return GetCamera().GetUp();
	}

	const CameraInterface& Device::GetCamera() const
	{
		auto maybe_id = level_->GetDefaultCameraId();
		if (!maybe_id) throw std::runtime_error("No camera?");
		auto scene_camera =
			dynamic_cast<NodeCamera*>(
				level_->GetSceneNodeFromId(maybe_id.value()));
		if (!scene_camera) throw std::runtime_error("Invalid cast.");
		return *scene_camera->GetCamera();
	}

	CameraInterface& Device::GetCamera()
	{
		auto maybe_id = level_->GetDefaultCameraId();
		if (!maybe_id) throw std::runtime_error("No camera?");
		auto scene_camera =
			dynamic_cast<NodeCamera*>(
				level_->GetSceneNodeFromId(maybe_id.value()));
		if (!scene_camera) throw std::runtime_error("Invalid cast.");
		return *scene_camera->GetCamera();
	}

} // End namespace frame::opengl.
