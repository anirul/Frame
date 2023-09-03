#pragma once

#include <frame/camera.h>
#include <frame/gui/draw_gui_interface.h>

namespace frame::gui {

	class WindowCamera : public frame::gui::GuiWindowInterface {
	public:
		WindowCamera(const std::string& name) : name_(name) {}
		void SetName(const std::string& name) override { name_ = name; }
		std::string GetName() const override { return name_; }
		void SetCameraPtr(frame::Camera* camera) { camera_ptr_ = camera; }
		frame::Camera* GetCameraPtr() const { return camera_ptr_; }
		bool End() const override { return end_; }
		void SaveCamera() { camera_saved_ = *camera_ptr_; }
		frame::Camera GetSavedCamera() const { return camera_saved_; }

	public:
		void RestoreCamera(float aspect_ratio);
		bool DrawCallback() override;

	private:
		glm::vec3 position_ = { 0, 0, 0 };
		glm::vec3 front_ = { 0, 0, -1 };
		glm::vec3 up_ = { 0, 1, 0 };
		glm::vec3 right_ = { 1, 0, 0 };
		float fov_degrees_ = 65.0f;
		float far_clip_ = 10000.0f;
		float near_clip_ = 0.1f;
		bool end_ = true;
		std::string name_;
		frame::Camera* camera_ptr_ = nullptr;
		frame::Camera camera_saved_;
		frame::Camera reset_camera_;
	};

}  // End namespace frame::gui.
