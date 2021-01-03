#pragma once

#include <memory>
#include <map>
#include <array>
#include <functional>
#include <optional>
#include <SDL2/SDL.h>
#include "../Frame/DeviceInterface.h"
#include "../Frame/Error.h"
#include "../Frame/Logger.h"
#include "../Frame/UniformInterface.h"
#include "../OpenGLLib/Program.h"
#include "../OpenGLLib/Texture.h"
#include "../OpenGLLib/Buffer.h"
#include "../OpenGLLib/StaticMesh.h"
#include "../OpenGLLib/Scene.h"
#include "../OpenGLLib/Camera.h"
#include "../OpenGLLib/Light.h"
#include "../OpenGLLib/Material.h"

namespace frame::opengl {

	class Device : 
		public DeviceInterface,
		public UniformInterface, 
		public std::enable_shared_from_this<Device>
	{
	public:
		// This will initialize the GL context and make the GLEW init.
		Device(
			void* gl_context, 
			const std::pair<std::uint32_t, std::uint32_t> size);

	public:
		// Startup the scene.
		void Startup(const std::shared_ptr<LevelInterface> level) override;
		// Cleanup the mess.
		void Cleanup() override;
		// Display the output texture to the display.
		void Display(const double dt) override;
		// Load scene from an OBJ file.
		void LoadSceneFromObjFile(const std::string& obj_file);

	public:
		const glm::mat4 GetProjection() const final { return perspective_; }
		const glm::mat4 GetView() const final { return view_; }
		const glm::mat4 GetModel() const final { return model_; }
		const double GetDeltaTime() const final { return dt_; }
		const CameraInterface& GetCamera() const final
		{
			return level_->GetSceneTree()->GetDefaultCamera();
		}
		CameraInterface& GetCamera() 
		{ 
			return level_->GetSceneTree()->GetDefaultCamera(); 
		}
		void* GetDeviceContext() const { return gl_context_; }
		const std::string GetTypeString() const { return "OpenGL"; }

	protected:
		void SetupCamera();

	private:
		std::shared_ptr<FrameBuffer> frame_ = nullptr;
		std::shared_ptr<RenderBuffer> render_ = nullptr;
		// Map of current stored level.
		std::shared_ptr<LevelInterface> level_ = nullptr;
		// Output texture (to the screen).
		std::string out_texture_name_ = "";
		// PVM matrices.
		glm::mat4 perspective_ = glm::mat4(1.0f);
		glm::mat4 view_ = glm::mat4(1.0f);
		glm::mat4 model_ = glm::mat4(1.0f);
		// Save dt locally per frame.
		double dt_ = 0.0f;
		// Open GL context.
		void* gl_context_ = nullptr;
		// Constants.
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const PixelElementSize pixel_element_size_ = PixelElementSize_HALF();
		// Cached quad and cube objects.
		std::shared_ptr<StaticMeshInterface> quad_ = nullptr;
		std::shared_ptr<StaticMeshInterface> cube_ = nullptr;
		// Error setup.
		const Error& error_ = Error::GetInstance();
		const Logger& logger_ = Logger::GetInstance();
	};

} // End namespace frame::opengl.
