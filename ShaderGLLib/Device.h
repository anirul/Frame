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
		Device(void* gl_context);

	public:
		// Startup the scene. Throw errors in case there is any.
		void Startup(const std::pair<int, int>& size);
		// Draw what is on the scene.
		// Take the total time from the beginning of the program to now as a
		// const double parameter.
		void Draw(const double dt);

	public:
		Camera GetCamera() const { return camera_; }
		void SetCamera(const sgl::Camera& camera) { camera_ = camera; }
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

	private:
		// Has to be a shared ptr as the program has to be created after the
		// window is present and the GLEW init is done.
		std::shared_ptr<sgl::Program> program_ = nullptr;
		sgl::SceneTree scene_tree_ = {};
		sgl::TextureManager texture_manager_ = {};
		sgl::LightManager light_manager_ = {};
		sgl::Camera camera_ = sgl::Camera({ 0.f, 0.f, 2.f });
		void* gl_context_ = nullptr;
		glm::mat4 perspective_ = glm::mat4(1.0f);
		glm::mat4 view_ = glm::mat4(1.0f);
		glm::mat4 model_ = glm::mat4(1.0f);
	};

} // End namespace sgl.
