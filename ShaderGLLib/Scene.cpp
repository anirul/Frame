#include "Scene.h"
#include <string>
#include <cassert>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include "Material.h"
#include "Convert.h"

namespace sgl {

	SceneMatrix::SceneMatrix(const frame::proto::SceneMatrix& proto_matrix)
	{
		if (proto_matrix.name().empty()) 
		{
			throw std::runtime_error("scene matrix should have a name.");
		}
		SetName(proto_matrix.name());
		SetParentName(proto_matrix.parent());
		matrix_ = ParseUniform(proto_matrix.matrix());
		euler_ = ParseUniform(proto_matrix.euler());
		quaternion_ = ParseUniform(proto_matrix.quaternion());
	}

	const glm::mat4 SceneMatrix::GetLocalModel(
		std::function<Ptr(const std::string&)> func,
		const double dt) const
	{
		if (!GetParentName().empty())
		{
			return 
				func(GetParentName())->GetLocalModel(func, dt) * 
				ComputeLocalRotation(dt);
		}
		return ComputeLocalRotation(dt);
	}

	const std::shared_ptr<sgl::StaticMesh> SceneMatrix::GetLocalMesh() const
	{
		return nullptr;
	}

	glm::mat4 SceneMatrix::ComputeLocalRotation(const double dt) const
	{
		// Check if we have a valid quaternion.
		if (quaternion_ != glm::quat(1, 0, 0, 0) &&
			quaternion_ != glm::quat(0, 0, 0, 0))
		{
			// Return the matrix multiplied by the rotation of the quaternion.
			return
				matrix_ *
				glm::toMat4(
					glm::mix(
						glm::quat(1, 0, 0, 0), 
						quaternion_, 
						static_cast<float>(dt)));
		}
		// Check if euler angler are valid (not 0).
		if (euler_ != glm::vec3(0.f, 0.f, 0.f))
		{
			return 
				matrix_ * 
				glm::toMat4(glm::quat(euler_ * static_cast<float>(dt)));
		}
		// Nothing to do return the basic matrix.
		return matrix_;
	}

	SceneStaticMesh::SceneStaticMesh(
		const frame::proto::SceneStaticMesh& proto_static_mesh)
	{
		SetName(proto_static_mesh.name());
		SetParentName(proto_static_mesh.parent());
		mesh_ = CreateStaticMeshFromObjFile(proto_static_mesh.file_name());
	}

	const glm::mat4 SceneStaticMesh::GetLocalModel(
		std::function<Ptr(const std::string&)> func,
		const double dt) const
	{
		if (!GetParentName().empty()) 
			return func(GetParentName())->GetLocalModel(func, dt);
		return glm::mat4(1.0f);
	}

	const std::shared_ptr<sgl::StaticMesh> SceneStaticMesh::GetLocalMesh() const
	{
		return mesh_;
	}

	SceneTree::SceneTree(const frame::proto::SceneTree& proto_tree)
	{
		name_ = proto_tree.name();
		root_node_name_ = proto_tree.root_node();
		if (root_node_name_.empty())
			throw std::runtime_error("cannot have an empty root node.");
		// Add scene matrices to the list.
		for (const auto& proto_matrix : proto_tree.scene_matrices())
		{
			scene_map_.insert(
				{ 
					proto_matrix.name(), 
					std::make_shared<SceneMatrix>(proto_matrix) 
				});
		}
		// Add scene static meshes to the list.
		for (const auto& proto_static_mesh : proto_tree.scene_static_meshes())
		{
			scene_map_.insert(
				{
					proto_static_mesh.name(),
					std::make_shared<SceneStaticMesh>(proto_static_mesh)
				});
		}
		// Add cameras to the list.
		for (const auto& proto_camera : proto_tree.scene_cameras())
		{
			scene_map_.insert(
				{
					proto_camera.name(),
					std::make_shared<SceneCamera>(proto_camera)
				});
		}
		// Add lights on the list.
		for (const auto& proto_light : proto_tree.scene_lights())
		{
			scene_map_.insert(
				{
					proto_light.name(),
					std::make_shared<SceneLight>(proto_light)
				});
		}
		// Check the root node is in the list.
		if (scene_map_.find(root_node_name_) == scene_map_.end())
		{
			throw std::runtime_error(
				"root node: " + root_node_name_ + " is not found!");
		}
		// Check we are all grounded to root node name!
		for (const auto& scene_pair : scene_map_)
		{
			// This is the root node so it should be ok!
			if (scene_pair.first == root_node_name_) continue;
			auto scene_node = scene_pair.second;
			std::uint32_t i = 0;
			while(true)
			{
				// Update the scene node.
				scene_node = scene_map_.at(scene_node->GetParentName());
				// Found the root node.
				if (scene_node->GetName() == root_node_name_) break;
				// End up in a trap!
				if (scene_node->GetName() == "")
				{
					throw std::runtime_error("Malformed scene tree!");
				}
				if (i++ > 0xffff)
				{
					throw std::runtime_error("Probably malformed scene tree.");
				}
			}
		}
	}

	const std::map<std::string, sgl::SceneInterface::Ptr> 
	SceneTree::GetSceneMap() const
	{
		return scene_map_;
	}

	const sgl::SceneInterface::Ptr SceneTree::GetSceneByName(
		const std::string& name) const
	{
		return scene_map_.at(name);
	}

	void SceneTree::AddNode(const SceneInterface::Ptr node)
	{
		scene_map_.insert({ node->GetName(), node });
	}

	const SceneInterface::Ptr SceneTree::GetRoot() const
	{
		SceneInterface::Ptr ret = nullptr;
		for (const auto& pair : scene_map_)
		{
			const auto& scene = pair.second;
			if (scene->GetParentName().empty())
			{
				if (ret)
					throw std::runtime_error("More than one root?");
				ret = scene;
			}
		}
		return ret;
	}

	void SceneTree::SetDefaultCamera(const std::string& camera_name)
	{
		camera_name_ = camera_name;
	}

	Camera& SceneTree::GetDefaultCamera()
	{
		auto ptr = GetSceneByName(camera_name_);
		if (ptr)
		{
			return dynamic_cast<SceneCamera*>(ptr.get())->GetCamera();
		}
		throw std::runtime_error("no camera in the scene.");
	}

	const sgl::Camera& SceneTree::GetDefaultCamera() const
	{
		auto ptr = GetSceneByName(camera_name_);
		if (ptr)
		{
			return dynamic_cast<SceneCamera*>(ptr.get())->GetCamera();
		}
		throw std::runtime_error("no camera in the scene.");
	}

	std::shared_ptr<SceneTree> LoadSceneFromObjStream(
		std::istream& is,
		const std::string& name) 
	{
		auto root_node = std::make_shared<SceneMatrix>(glm::mat4(1.0f));
		auto scene_tree = std::make_shared<SceneTree>();
		scene_tree->AddNode(root_node);
		// Open the OBJ file.
		std::string obj_text = "";
		std::string obj_name = "";
		auto lambda_create_mesh = 
			[&obj_text, &obj_name, &scene_tree, &root_node]() 
		{
			std::istringstream obj_iss(obj_text);
			auto mesh = std::make_shared<StaticMesh>(obj_iss, obj_name);
			auto mesh_node = std::make_shared<SceneStaticMesh>(mesh);
			const auto& root_name = root_node->GetName();
			mesh_node->SetName(obj_name);
			mesh_node->SetParentName(root_name);
			scene_tree->AddNode(mesh_node);
			obj_text.clear();
			obj_name.clear();
		};
		while (!is.eof())
		{
			std::string line = "";
			if (!std::getline(is, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
			{
				throw std::runtime_error(
					"Error parsing file: " + name + " no token found.");
			}
			if (dump[0] == '#') continue;
			if (dump == "mtllib")
			{
				throw std::runtime_error(
					"Parsing a \"mtllib\" in: " + name);
			}
			if (dump == "o")
			{
				if (!obj_text.empty() && !obj_name.empty())
					lambda_create_mesh();
				std::string mesh_name;
				if (!(iss >> mesh_name))
				{
					throw std::runtime_error(
						"Error parsing file: " + name + 
						" no token found.");
				}
				obj_name = mesh_name;
				continue;
			}
			obj_text += line + "\n";
		}
		if (!obj_text.empty() && !obj_name.empty())
			lambda_create_mesh();
		return scene_tree;
	}

	SceneCamera::SceneCamera(const frame::proto::SceneCamera& proto_camera) :
		camera_(
			ParseUniform(proto_camera.position()),
			ParseUniform(proto_camera.target()),
			ParseUniform(proto_camera.up()),
			proto_camera.fov_degrees())	
	{
		SetName(proto_camera.name());
		SetParentName(proto_camera.parent());
	}

	const glm::mat4 SceneCamera::GetLocalModel(
		std::function<Ptr(const std::string&)> func,
		const double dt) const
	{
		if (!GetParentName().empty())
			return func(GetParentName())->GetLocalModel(func, dt);
		return glm::mat4(1.0f);
	}

	const std::shared_ptr<sgl::StaticMesh> SceneCamera::GetLocalMesh() const
	{
		return nullptr;
	}

	SceneLight::SceneLight(const frame::proto::SceneLight& proto_light)
	{
		SetName(proto_light.name());
		SetParentName(proto_light.parent());
		light_type_ = proto_light.light_type();
		position_ = ParseUniform(proto_light.position());
		direction_ = ParseUniform(proto_light.direction());
		color_ = ParseUniform(proto_light.color());
		dot_inner_limit_ = proto_light.dot_inner_limit();
		dot_outer_limit_ = proto_light.dot_outer_limit();
	}

	SceneLight::SceneLight(
		const frame::proto::SceneLight::LightEnum light_type, 
		const glm::vec3 position_or_direction, 
		const glm::vec3 color) :
		light_type_(light_type),
		color_(color)
	{
		if (light_type_ == frame::proto::SceneLight::POINT)
		{
			position_ = position_or_direction;
		}
		else if (light_type_ == frame::proto::SceneLight::DIRECTIONAL)
		{
			direction_ = position_or_direction;
		}
		else
		{
			std::string value = std::to_string(static_cast<int>(light_type));
			throw std::runtime_error("illegal light(" + value + ")");
		}
	}

	SceneLight::SceneLight(
		const glm::vec3 position, 
		const glm::vec3 direction, 
		const glm::vec3 color, 
		const float dot_inner_limit, 
		const float dot_outer_limit) :
		light_type_(frame::proto::SceneLight::SPOT),
		position_(position),
		direction_(direction),
		color_(color),
		dot_inner_limit_(dot_inner_limit),
		dot_outer_limit_(dot_outer_limit) {}

	const glm::mat4 SceneLight::GetLocalModel(
		std::function<Ptr(const std::string&)> func,
		const double dt) const
	{
		if (!GetParentName().empty())
			return func(GetParentName())->GetLocalModel(func, dt);
		return glm::mat4(1.0f);
	}

	const std::shared_ptr<sgl::StaticMesh> SceneLight::GetLocalMesh() const
	{
		return nullptr;
	}

} // End namespace sgl.
