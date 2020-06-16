#pragma once

#include <memory>
#include <map>
#include <array>
#include <functional>
#include <optional>
#include <SDL2/SDL.h>
#include "Program.h"
#include "Texture.h"
#include "../ShaderGLLib/Buffer.h"
#include "../ShaderGLLib/Mesh.h"
#include "../ShaderGLLib/Scene.h"
#include "../ShaderGLLib/Camera.h"
#include "../ShaderGLLib/Light.h"
#include "../ShaderGLLib/Material.h"

namespace sgl {

	class Device
	{
	public:
		// This will initialize the GL context and make the GLEW init.
		Device(
			void* gl_context, 
			const std::pair<std::uint32_t, std::uint32_t> size);

	public:
		// Startup the scene. Throw errors in case there is any, takes fov in 
		// degrees.
		void Startup(const float fov = 65.0f);
		// Draw what is on the scene.
		// Take the total time from the beginning of the program to now as a
		// const double parameter.
		void Draw(const double dt);
		// Draw to the deferred texture set.
		void DrawDeferred(
			const double dt,
			const std::vector<std::shared_ptr<Texture>>& 
				deferred_textures = {});
		void DrawView(
			const double dt,
			const std::vector<std::shared_ptr<Texture>>& view_textures = {});
		// Draw the lighting texture from either the inside deferred textures or
		// from the provided deferred textures.
		std::shared_ptr<Texture> DrawLighting(
			const std::vector<std::shared_ptr<Texture>>& in_textures = {});
		// Create a screen space ambient occlusion from either the texture
		// passed or the one from the physically based rendering path.
		std::shared_ptr<Texture> DrawScreenSpaceAmbientOcclusion(
			const std::vector<std::shared_ptr<Texture>>& in_textures = {});
		// Add Bloom to the provided texture.
		std::shared_ptr<Texture> DrawBloom(
			const std::shared_ptr<Texture>& texture);
		// Add HDR to a texture (with associated gamma and exposure).
		std::shared_ptr<Texture> DrawHighDynamicRange(
			const std::shared_ptr<Texture>& texture,
			const float exposure = 1.0f,
			const float gamma = 2.2f);
		// Draw to a texture.
		std::shared_ptr<Texture> DrawTexture(const double dt);
		// Draw to multiple textures.
		void DrawMultiTextures(
			const double dt,
			const std::vector<std::shared_ptr<Texture>>& out_textures,
			const std::shared_ptr<Program> program = nullptr);
		void AddEnvironment(const std::string& environment_map);
		// Display a texture to the display.
		void Display(const std::shared_ptr<Texture>& texture);
		// Load scene from an OBJ file.
		void LoadSceneFromObjFile(const std::string& obj_file);
		// Debug access to the internals of device.
		const std::shared_ptr<Texture>& GetDeferredTexture(const int i) const;
		const std::shared_ptr<Texture>& GetViewTexture(const int i) const;
		const std::shared_ptr<Texture>& GetLightingTexture(const int i) const;
		const std::shared_ptr<Texture>& GetNoiseTexture() const;

	public:
		Camera GetCamera() const { return camera_; }
		void SetCamera(const Camera& camera) { camera_ = camera; }
		SceneTree GetSceneTree() const { return scene_tree_; }
		void SetSceneTree(const SceneTree& scene_tree) 
		{ 
			scene_tree_ = scene_tree;
		}
		TextureManager GetTextureManager() { return texture_manager_; }
		void SetTextureManager(const TextureManager& texture_manager)
		{
			texture_manager_ = texture_manager;
		}
		LightManager GetLightManager() const { return light_manager_; }
		void SetLightManager(const LightManager& light_manager)
		{
			light_manager_ = light_manager;
		}
		const glm::mat4 GetProjection() const { return perspective_; }
		const glm::mat4 GetView() const { return view_; }
		const glm::mat4 GetModel() const { return model_; }
		void* GetDeviceContext() const { return gl_context_; }

	protected:
		void SetupCamera();

	private:
		std::shared_ptr<Program> pbr_program_ = nullptr;
		std::shared_ptr<Program> lighting_program_ = nullptr;
		std::shared_ptr<Program> ssao_program_ = nullptr;
		std::shared_ptr<Program> view_program_ = nullptr;
		std::shared_ptr<Program> blur_program_ = nullptr;
		std::shared_ptr<Texture> noise_texture_ = nullptr;
		std::vector<glm::vec3> kernel_ssao_vec_ = {};
		std::vector<std::shared_ptr<Texture>> deferred_textures_ = {};
		std::vector<std::shared_ptr<Texture>> lighting_textures_ = {};
		std::vector<std::shared_ptr<Texture>> view_textures_ = {};
		std::shared_ptr<Texture> final_texture_ = nullptr;
		std::map<std::string, std::shared_ptr<Material>> materials_ = {};
		SceneTree scene_tree_ = {};
		TextureManager texture_manager_ = {};
		LightManager light_manager_ = {};
		// Camera storage.
		Camera camera_ = Camera({ 0.1f, 5.f, -7.f }, { -0.1f, -1.f, 2.f });
		// PVM matrices.
		glm::mat4 perspective_ = glm::mat4(1.0f);
		glm::mat4 view_ = glm::mat4(1.0f);
		glm::mat4 model_ = glm::mat4(1.0f);
		// Field of view (in degrees).
		float fov_ = 65.f;
		// Open GL context.
		void* gl_context_ = nullptr;
		// Constants.
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const PixelElementSize pixel_element_size_ = PixelElementSize::HALF;
		// Error setup.
		const Error& error_ = Error::GetInstance();
	};

} // End namespace sgl.
