#pragma once

#include <memory>
#include <map>
#include <array>
#include <functional>
#include <optional>
#include <SDL2/SDL.h>
#include "../ShaderGLLib/Program.h"
#include "../ShaderGLLib/Texture.h"
#include "../ShaderGLLib/Buffer.h"
#include "../ShaderGLLib/Effect.h"
#include "../ShaderGLLib/StaticMesh.h"
#include "../ShaderGLLib/Scene.h"
#include "../ShaderGLLib/Camera.h"
#include "../ShaderGLLib/Light.h"
#include "../ShaderGLLib/Material.h"
#include "../ShaderGLLib/Uniform.h"

namespace sgl {

	class Device : 
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
		void Startup(
			const frame::proto::Level& proto_level,
			const frame::proto::EffectFile& proto_effect_file, 
			const frame::proto::SceneFile& proto_scene_file,
			const frame::proto::TextureFile& proto_texture_file);
		// Draw to multiple textures.
		// Take the total time from the beginning of the program to now as a
		// const double parameter.
		void DrawMultiTextures(
			const std::shared_ptr<ProgramInterface> program,
			const std::vector<std::shared_ptr<Texture>>& out_textures,
			const double dt);
		// CHECKME(anirul): Is it really needed?
		// Add environment to the scene.
		void AddEnvironment(const std::string& environment_map);
		// Display the output texture to the display.
		void Display(const double dt);
		// Load scene from an OBJ file.
		void LoadSceneFromObjFile(const std::string& obj_file);

	public:
		const glm::mat4 GetProjection() const final { return perspective_; }
		const glm::mat4 GetView() const final { return view_; }
		const glm::mat4 GetModel() const final { return model_; }
		const double GetDeltaTime() const final { return dt_; }
		const Camera& GetCamera() const final;
		Camera& GetCamera() { return scene_tree_->GetDefaultCamera(); }
		void* GetDeviceContext() const { return gl_context_; }
		const std::string GetType() const { return "OpenGL"; }

	protected:
		void SetupCamera();

	private:
		std::shared_ptr<Frame> frame_ = nullptr;
		std::shared_ptr<Render> render_ = nullptr;
		// CHECKME(anirul): Is it really needed?
		std::shared_ptr<Material> environment_material_ = nullptr;
		std::map<std::string, std::shared_ptr<Effect>> effect_map_ = {};
		std::map<std::string, std::shared_ptr<Material>> material_map_ = {};
		std::map<std::string, std::shared_ptr<Texture>> texture_map_ = {};
		std::shared_ptr<SceneTree> scene_tree_ = nullptr;
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
		std::shared_ptr<StaticMesh> quad_ = nullptr;
		std::shared_ptr<StaticMesh> cube_ = nullptr;
		// Error setup.
		const Error& error_ = Error::GetInstance();
		const Logger& logger_ = Logger::GetInstance();
	};

} // End namespace sgl.
