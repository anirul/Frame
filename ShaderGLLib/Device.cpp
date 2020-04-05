#include "Device.h"
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

namespace sgl {

	Device::Device(void* gl_context) : gl_context_(gl_context) 
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
	}

	void Device::Startup(const std::pair<int, int>& size)
	{
		// Set the perspective matrix.
		const float aspect =
			static_cast<float>(size.first) / static_cast<float>(size.second);
		perspective_ = glm::perspective(
			glm::radians(65.0f),
			aspect,
			0.1f,
			100.0f);
		
		// Set the camera.
		view_ = camera_.GetLookAt();
	}

	void Device::Draw(const double dt)
	{
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
			mesh->Draw(texture_manager_, scene->GetLocalModel(dt));
		}
	}

} // End namespace sgl.
