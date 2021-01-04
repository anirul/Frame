#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <functional>
#include <glm/gtx/quaternion.hpp>
#include "Frame/OpenGL/StaticMesh.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/Camera.h"
#include "Frame/SceneNodeInterface.h"
#include "Frame/SceneTreeInterface.h"

namespace frame::opengl {

	class SceneMatrix : public SceneNodeInterface
	{
	public:
		SceneMatrix(const glm::mat4 matrix) : matrix_(matrix) {}
		SceneMatrix(const glm::mat4 matrix, const glm::vec3 euler) :
			matrix_(matrix), euler_(euler) {}
		SceneMatrix(const glm::mat4 matrix, const glm::quat quaternion) :
			matrix_(matrix), quaternion_(quaternion) {}
		SceneMatrix(const frame::proto::SceneMatrix& proto_matrix);

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<StaticMeshInterface> 
			GetLocalMesh() const override;

	protected:
		glm::mat4 ComputeLocalRotation(const double dt) const;

	private:
		glm::mat4 matrix_ = glm::mat4(1.f);
		glm::vec3 euler_ = { 0.f, 0.f, 0.f };
		glm::quat quaternion_ = { 1.f, 0.f, 0.f, 0.f };
	};

	class SceneStaticMesh : public SceneNodeInterface
	{
	public:
		SceneStaticMesh(std::shared_ptr<StaticMesh> mesh) : mesh_(mesh) {}
		SceneStaticMesh(const frame::proto::SceneStaticMesh& proto_static_mesh);

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<StaticMeshInterface> 
			GetLocalMesh() const override; 

	private:
		std::shared_ptr<StaticMesh> mesh_ = nullptr;
	};

	class SceneCamera : public SceneNodeInterface
	{
	public:
		SceneCamera(
			const glm::vec3 position = glm::vec3{ 0.f, 0.f, 0.f }, 
			const glm::vec3 target = glm::vec3{ 0.f, 0.f, -1.f }, 
			const glm::vec3 up = glm::vec3{ 0.f, 1.f, 0.f },
			const float fov_degrees = 65.0f) : 
			camera_(position, target, up, fov_degrees) {}
		SceneCamera(const frame::proto::SceneCamera& proto_camera);

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<StaticMeshInterface> 
			GetLocalMesh() const override;

	public:
		Camera& GetCamera() { return camera_; }
		const Camera& GetCamera() const { return camera_; }

	private:
		Camera camera_ = {};
	};

	class SceneLight : public SceneNodeInterface
	{
	public:
		// Create an ambient light.
		SceneLight(const glm::vec3 color) : 
			light_type_(frame::proto::SceneLight::AMBIENT), color_(color) {}
		// Create a point or directional light.
		SceneLight(
			const frame::proto::SceneLight::LightEnum light_type, 
			const glm::vec3 position_or_direction, 
			const glm::vec3 color);
		// Create a spot light.
		SceneLight(
			const glm::vec3 position,
			const glm::vec3 direction,
			const glm::vec3 color,
			const float dot_inner_limit,
			const float dot_outer_limit);
		// From proto.
		SceneLight(const frame::proto::SceneLight& proto_light);

	public:
		const glm::mat4 GetLocalModel(const double dt) const override;
		const std::shared_ptr<StaticMeshInterface> 
			GetLocalMesh() const override;

	public:
		const frame::proto::SceneLight::LightEnum GetType() const 
		{ 
			return light_type_; 
		}
		const glm::vec3 GetPosition() const { return position_; }
		const glm::vec3 GetDirection() const { return direction_; }
		const glm::vec3 GetColor() const { return color_; }
		const float GetDotInner() const { return dot_inner_limit_; }
		const float GetDotOuter() const { return dot_outer_limit_; }

	private:
		frame::proto::SceneLight::LightEnum light_type_ = 
			frame::proto::SceneLight::INVALID;
		glm::vec3 position_ = glm::vec3(0.0f);
		glm::vec3 direction_ = glm::vec3(0.0f);
		glm::vec3 color_ = glm::vec3(1.0f);
		float dot_inner_limit_ = 0.0f;
		float dot_outer_limit_ = 0.0f;
	};

	class SceneTree : public SceneTreeInterface
	{
	public:
		// Create a default empty scene tree. 
		SceneTree() = default;
		// Create a scene tree from a proto file.
		SceneTree(const frame::proto::SceneTree& proto_tree);

	public:
		// Return a map of scene names and scene components.
		const std::map<std::string, SceneNodeInterface::Ptr> 
			GetSceneMap() const override;
		// Return the element at name position (or nullptr).
		const SceneNodeInterface::Ptr GetSceneByName(
			const std::string& name) const override;
		// Add a node to the scene tree. This will also add the callback to the
		// node to the GetSceneByName function.
		void AddNode(const SceneNodeInterface::Ptr node) override;
		// Get the root of the scene tree.
		const SceneNodeInterface::Ptr GetRoot() const override;
		// Set the default camera node.
		void SetDefaultCamera(const std::string& camera_name) override;
		// Get a pointer to the default camera.
		CameraInterface& GetDefaultCamera() override;
		// Same but const version.
		const CameraInterface& GetDefaultCamera() const override;

	private:
		// Contain the scene.
		std::map<std::string, SceneNodeInterface::Ptr> scene_map_;
		// Name of the scene.
		std::string name_;
		// Name of the root node.
		std::string root_node_name_;
		// Store the default camera node.
		std::string camera_name_;
	};

	std::shared_ptr<SceneTreeInterface> LoadSceneFromObjStream(
		std::istream& is, 
		const std::string& name);

} // End namespace sgl.
