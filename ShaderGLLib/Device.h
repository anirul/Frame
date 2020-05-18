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
		// Draw to a texture.
		std::shared_ptr<Texture> DrawTexture(const double dt);
		// Display a texture to the display.
		void Display(const std::shared_ptr<Texture>& texture);

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
		SceneTree scene_tree_ = {};
		TextureManager texture_manager_ = {};
		LightManager light_manager_ = {};
		Camera camera_ = Camera({ 0.f, 0.f, 2.f }, { 0.f, 0.f, 0.f });
		glm::mat4 perspective_ = glm::mat4(1.0f);
		glm::mat4 view_ = glm::mat4(1.0f);
		glm::mat4 model_ = glm::mat4(1.0f);
		float fov_ = 65.f;
		void* gl_context_ = nullptr;
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		// Error setup.
		const Error& error_ = Error::GetInstance();
	};

} // End namespace sgl.
