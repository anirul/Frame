#include "Scene.h"
#include <string>
#include <cassert>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include "Material.h"

namespace sgl {

	const glm::mat4 SceneMatrix::GetLocalModel(const double dt) const
	{
		if (parent_)
		{
			return parent_->GetLocalModel(dt) * func_(dt);
		}
		else
		{
			return func_(dt);
		}
	}

	const std::shared_ptr<sgl::Mesh> SceneMatrix::GetLocalMesh() const
	{
		return nullptr;
	}

	const glm::mat4 SceneMesh::GetLocalModel(const double dt) const
	{
		if (parent_)
		{
			return parent_->GetLocalModel(dt);
		}
		else
		{
			return glm::mat4(1.0f);
		}
	}

	const std::shared_ptr<sgl::Mesh> SceneMesh::GetLocalMesh() const
	{
		return mesh_;
	}

	void SceneTree::AddNode(
		const std::shared_ptr<Scene>& node, 
		const std::shared_ptr<Scene>& parent /*= nullptr*/)
	{
		node->SetParent(parent);
		push_back(node);
	}

	const std::shared_ptr<Scene> SceneTree::GetRoot() const
	{
		std::shared_ptr<Scene> ret;
		for (const auto& scene : *this)
		{
			if (!scene->GetParent())
			{
				if (ret)
				{
					throw std::runtime_error("More than one root?");
				}
				ret = scene;
			}
		}
		return ret;
	}

	SceneTree LoadSceneFromObjStream(
		std::istream& is,
		const std::shared_ptr<Program>& program,
		const std::string& name) 
	{
		auto root_node = std::make_shared<SceneMatrix>(glm::mat4(1.0f));
		SceneTree scene_tree = {};
		scene_tree.AddNode(root_node);
		// Open the OBJ file.
		std::string obj_text = "";
		std::string obj_name = "";
		auto lambda_create_mesh = 
			[&obj_text, &obj_name, &scene_tree, &root_node, &program]() 
		{
			std::istringstream obj_iss(obj_text);
			auto mesh = std::make_shared<Mesh>(obj_iss, obj_name, program);
			auto mesh_node = std::make_shared<SceneMesh>(mesh);
			mesh_node->SetParent(root_node);
			scene_tree.AddNode(mesh_node, root_node);
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

} // End namespace sgl.
