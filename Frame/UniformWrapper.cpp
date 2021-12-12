#include "Frame/UniformWrapper.h"

namespace frame {

	glm::vec3 UniformWrapper::GetCameraPosition() const
	{
		return camera_->GetPosition();
	}

	glm::vec3 UniformWrapper::GetCameraFront() const
	{
		return camera_->GetFront();
	}

	glm::vec3 UniformWrapper::GetCameraRight() const
	{
		return camera_->GetRight();
	}

	glm::vec3 UniformWrapper::GetCameraUp() const
	{
		return camera_->GetUp();
	}

	glm::mat4 UniformWrapper::GetProjection() const
	{
		return camera_->ComputeProjection();
	}

	glm::mat4 UniformWrapper::GetView() const
	{
		return camera_->ComputeView();
	}

	glm::mat4 UniformWrapper::GetModel() const
	{
		return model_;
	}

	double UniformWrapper::GetDeltaTime() const
	{
		return time_;
	}

} // End namespace frame.
