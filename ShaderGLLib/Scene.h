#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <functional>
#include <glm/gtx/quaternion.hpp>
#include "../ShaderGLLib/StaticMesh.h"
#include "../ShaderGLLib/Material.h"
#include "../ShaderGLLib/Camera.h"

namespace sgl {

	// Interface to visit the scene tree.
	class SceneInterface
	{
	public:
		// Redefinition for shortening.
		using Ptr = std::shared_ptr<SceneInterface>;
		using PtrVec = std::vector<std::shared_ptr<SceneInterface>>;

	public:
		// Get the local model of current node, As an input take a function
		// that return a pointer to the parent from a string, this can be a
		// lambda.
		virtual const glm::mat4 GetLocalModel(
			std::function<Ptr(const std::string&)> func, 
			double dt) const = 0;
		// Get the local mesh of current node.
		virtual const std::shared_ptr<StaticMesh> GetLocalMesh() const = 0;

	public:
		// Return true if this is the root node (no parents).
		bool IsRoot() const { return GetParentName().empty(); }
		// Get the parent of a node.
		const std::string GetParentName() const { return parent_name_; }
		// Set the parent of a node.
		void SetParentName(const std::string& parent) { parent_name_ = parent; }
		// Getter for name.
		const std::string GetName() const { return name_; }
		// Setter for name.
		void SetName(const std::string& name) { name_ = name; }

	protected:
		std::string parent_name_;
		std::string name_ = "";
	};

	class SceneMatrix : public SceneInterface
	{
	public:
		SceneMatrix(const glm::mat4 matrix) : matrix_(matrix) {}
		SceneMatrix(const glm::mat4 matrix, const glm::vec3 euler) :
			matrix_(matrix), euler_(euler) {}
		SceneMatrix(const glm::mat4 matrix, const glm::quat quaternion) :
			matrix_(matrix), quaternion_(quaternion) {}
		SceneMatrix(const frame::proto::SceneMatrix& proto_matrix);

	public:
		const glm::mat4 GetLocalModel(
			std::function<Ptr(const std::string&)> func, 
			const double dt) const override;
		const std::shared_ptr<StaticMesh> GetLocalMesh() const override;

	protected:
		glm::mat4 ComputeLocalRotation(const double dt) const;

	private:
		glm::mat4 matrix_ = glm::mat4(1.f);
		glm::vec3 euler_ = { 0.f, 0.f, 0.f };
		glm::quat quaternion_ = { 1.f, 0.f, 0.f, 0.f };
	};

	class SceneStaticMesh : public SceneInterface
	{
	public:
		SceneStaticMesh(std::shared_ptr<StaticMesh> mesh) : mesh_(mesh) {}
		SceneStaticMesh(const frame::proto::SceneStaticMesh& proto_static_mesh);

	public:
		const glm::mat4 GetLocalModel(
			std::function<Ptr(const std::string&)> func, 
			const double dt) const override;
		const std::shared_ptr<StaticMesh> GetLocalMesh() const override; 

	private:
		std::shared_ptr<StaticMesh> mesh_ = nullptr;
	};

	class SceneCamera : public SceneInterface
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
		const glm::mat4 GetLocalModel(
			std::function<Ptr(const std::string&)> func, 
			const double dt) const override;
		const std::shared_ptr<StaticMesh> GetLocalMesh() const override;

	public:
		Camera& GetCamera() { return camera_; }
		const Camera& GetCamera() const { return camera_; }

	private:
		Camera camera_ = {};
	};

	class SceneLight : public SceneInterface
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
		const glm::mat4 GetLocalModel(
			std::function<Ptr(const std::string&)> func, 
			const double dt) const override;
		const std::shared_ptr<StaticMesh> GetLocalMesh() const override;

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

	class SceneTree
	{
	public:
		// Create a default empty scene tree. 
		SceneTree() = default;
		// Create a scene tree from a proto file.
		SceneTree(const frame::proto::SceneTree& proto_tree);

	public:
		// Return a map of scene names and scene components.
		const std::map<std::string, SceneInterface::Ptr> GetSceneMap() const;
		// Return the element at name position (or nullptr).
		const SceneInterface::Ptr GetSceneByName(const std::string& name) const;
		// Add a node to the scene tree.
		void AddNode(const SceneInterface::Ptr node);
		// Get the root of the scene tree.
		const SceneInterface::Ptr GetRoot() const;
		// Set the default camera node.
		void SetDefaultCamera(const std::string& camera_name);
		// Get a pointer to the default camera.
		Camera& GetDefaultCamera();
		// Same but const version.
		const Camera& GetDefaultCamera() const;

	private:
		// Contain the scene.
		std::map<std::string, SceneInterface::Ptr> scene_map_;
		// Name of the scene.
		std::string name_;
		// Name of the root node.
		std::string root_node_name_;
		// Store the default camera node.
		std::string camera_name_;
	};

	std::shared_ptr<SceneTree> LoadSceneFromObjStream(
		std::istream& is, 
		const std::string& name);

} // End namespace sgl.
