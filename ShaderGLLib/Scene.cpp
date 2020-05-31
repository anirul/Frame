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

	SceneTreeMaterial LoadSceneFromObjFile(
		const std::string& obj_file, 
		const std::shared_ptr<Program>& program)
	{
		if (obj_file.empty())
		{
			throw std::runtime_error(
				"Error invalid file name: " + obj_file);
		}
		std::string mtl_file = "";
		std::string mtl_path = obj_file;
		while (mtl_path.back() != '/' || mtl_path.back() != '\\')
		{
			mtl_path.pop_back();
		}
		std::ifstream obj_ifs(obj_file);
		if (!obj_ifs.is_open())
		{
			throw std::runtime_error(
				"Could not open file: " + obj_file);
		}
		std::string obj_content = "";
		while (!obj_ifs.eof())
		{
			std::string line = "";
			if (!std::getline(obj_ifs, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
			{
				throw std::runtime_error(
					"Error parsing file: " + obj_file);
			}
			if (dump[0] == '#') continue;
			if (dump == "mtllib")
			{
				if (!(iss >> mtl_file))
				{
					mtl_file = mtl_path + mtl_file;
				}
				continue;
			}
			obj_content += line + "\n";
		}
		std::ifstream mtl_ifs(mtl_file);
		if (!mtl_ifs.is_open())
		{
			throw std::runtime_error(
				"Error cannot open file: " + mtl_file);
		}
		SceneTreeMaterial scene_tree_material{};
		scene_tree_material.scene_tree =
			LoadSceneFromObjStream(
				std::istringstream(obj_content), 
				program, 
				obj_file);
		scene_tree_material.materials =
			LoadMaterialFromMtlStream(mtl_ifs, mtl_file);
		return scene_tree_material;
	}

	std::shared_ptr<sgl::SceneTree> LoadSceneFromObjStream(
		std::istream& is,
		const std::shared_ptr<Program>& program,
		const std::string& name) 
	{
		auto root_node = std::make_shared<SceneMatrix>(glm::mat4(1.0f));
		auto scene_tree = std::make_shared<SceneTree>();
		scene_tree->AddNode(root_node);
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
			scene_tree->AddNode(mesh_node);
		};
		while (!is.eof())
		{
			std::string line = "";
			if (std::getline(is, line)) break;
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
