#pragma once

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <SDL2/SDL.h>
#include "Frame/Camera.h"
#include "Frame/DeviceInterface.h"
#include "Frame/Error.h"
#include "Frame/Logger.h"
#include "Frame/UniformInterface.h"
#include "Frame/OpenGL/Program.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/OpenGL/Buffer.h"
#include "Frame/OpenGL/StaticMesh.h"
#include "Frame/OpenGL/Light.h"
#include "Frame/OpenGL/Renderer.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/NodeCamera.h"

namespace frame::opengl {

	class Device : public DeviceInterface, public UniformInterface
	{
	public:
		// This will initialize the GL context and make the GLEW init.
		Device(
			void* gl_context, 
			const std::pair<std::uint32_t, std::uint32_t> size);
		virtual ~Device();

	public:
		const CameraInterface& GetCamera() const final;
		CameraInterface& GetCamera() final;
		// Startup the scene.
		void Startup(std::unique_ptr<LevelInterface>&& level) final;
		// Cleanup the mess.
		void Cleanup() final;
		// Display the output texture to the display.
		void Display(const double dt) final;
		// Some implementation from in heritage.
		const glm::vec3 GetCameraFront() const final;
		const glm::vec3 GetCameraRight() const final;
		const glm::vec3 GetCameraUp() const final;
		const glm::vec3 GetCameraPosition() const final;

	public:
		const double GetDeltaTime() const final { return dt_; }
		const glm::mat4 GetProjection() const final { return projection_; }
		const glm::mat4 GetView() const final { return view_; }
		const glm::mat4 GetModel() const final { return model_; }
		void* GetDeviceContext() const final { return gl_context_; }
		const std::string GetTypeString() const final { return "OpenGL"; }

	protected:
		void SetupCamera();
		bool HasNameInParents(EntityId node_id, const std::string& name) const;

	private:
		// Map of current stored level.
		std::unique_ptr<LevelInterface> level_ = nullptr;
		// Order of program to be rendered (fixed by input output).
		// The order is relevant and this should also be a set as program
		// should only be referred once.
		std::vector<EntityId> program_render_ = {};
		// Output texture (to the screen).
		std::string out_texture_name_ = "";
		// PVM matrices.
		glm::mat4 projection_ = glm::mat4(1.0f);
		glm::mat4 view_ = glm::mat4(1.0f);
		glm::mat4 model_ = glm::mat4(1.0f);
		// Save dt locally per frame.
		double dt_ = 0.0f;
		// Open GL context.
		void* gl_context_ = nullptr;
		// Constants.
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const proto::PixelElementSize pixel_element_size_ = 
			proto::PixelElementSize_HALF();
		// Rendering pipeline.
		std::unique_ptr<Renderer> renderer_ = nullptr;
		// Error setup.
		const Error& error_ = Error::GetInstance();
		const Logger& logger_ = Logger::GetInstance();
	};

} // End namespace frame::opengl.
