#include "Device.h"
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

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
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// Enable Z buffer.
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		// Enable seamless cube map.
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}

	void Device::Startup(const float fov /*= 65.0f*/)
	{
		fov_ = fov;
		SetupCamera();
	}

	void Device::Draw(const double dt)
	{
		// Setup the camera.
		SetupCamera();

		// Set the view port for rendering.
		glViewport(0, 0, size_.first, size_.second);

		// Clear the screen.
		glClearColor(.2f, 0.f, .2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (const std::shared_ptr<sgl::Scene>& scene : scene_tree_)
		{
			const std::shared_ptr<sgl::Mesh>& mesh = scene->GetLocalMesh();
			if (!mesh)
			{
				continue;
			}

			// Draw the mesh.
			mesh->Draw(
				texture_manager_, 
				perspective_, 
				view_, 
				scene->GetLocalModel(dt));
		}
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

} // End namespace sgl.
