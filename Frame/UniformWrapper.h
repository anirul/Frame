#pragma once

#include "Frame/LevelInterface.h"
#include "Frame/UniformInterface.h"

namespace frame {

	class UniformWrapper : public UniformInterface 
	{
	public:
		UniformWrapper(CameraInterface* camera_interface) 
		{
			camera_ = camera_interface;
		}

	public:
		void SetModel(glm::mat4 model) { model_ = model; }
		void SetTime(double time) { time_ = time; }

	public:
		glm::vec3 GetCameraPosition() const override;
		glm::vec3 GetCameraFront() const override;
		glm::vec3 GetCameraRight() const override;
		glm::vec3 GetCameraUp() const override;
		glm::mat4 GetProjection() const override;
		glm::mat4 GetView() const override;
		glm::mat4 GetModel() const override;
		double GetDeltaTime() const override;

	private:
		CameraInterface* camera_ = nullptr;
		glm::mat4 model_ = glm::mat4(1.0f);
		double time_ = 0.0;
	};

} // End namespace frame.
