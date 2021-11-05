#pragma once

#include <glm/glm.hpp>
#include "Frame/Camera.h"
#include "Frame/NodeInterface.h"

namespace frame {

	class NodeCamera : public NodeInterface
	{
	public:
		NodeCamera(
			std::function<NodeInterface*(const std::string&)> func,
			const glm::vec3 position = glm::vec3{ 0.f, 0.f, 0.f },
			const glm::vec3 target = glm::vec3{ 0.f, 0.f, -1.f },
			const glm::vec3 up = glm::vec3{ 0.f, 1.f, 0.f },
			const float fov_degrees = 65.0f) :
			NodeInterface(func),
			camera_(std::make_shared<Camera>(position, target, up, fov_degrees))
		{}

	public:
		glm::mat4 GetLocalModel(const double dt) const override;

	public:
		std::shared_ptr<CameraInterface> GetCamera() { return camera_; }
		const std::shared_ptr<CameraInterface> GetCamera() const 
		{ 
			return camera_; 
		}

	private:
		std::shared_ptr<CameraInterface> camera_ = nullptr;
	};

} // End namespace frame.
