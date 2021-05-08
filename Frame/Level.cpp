#include "Level.h"

namespace frame {

	EntityId Level::AddSceneNode(
		const std::string& name, 
		std::shared_ptr<NodeInterface> scene_node)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		EntityId id = GetSceneNodeNewId();
		string_set_.insert(name);
		id_scene_node_map_.insert({ id, scene_node });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	EntityId Level::AddTexture(
		const std::string& name,
		std::shared_ptr<TextureInterface> texture)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		EntityId id = GetTextureNewId();
		string_set_.insert(name);
		id_texture_map_.insert({ id, texture });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	EntityId Level::AddProgram(
		const std::string& name,
		std::shared_ptr<ProgramInterface> program)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		EntityId id = GetProgramNewId();
		id_program_map_.insert({ id, program });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	EntityId Level::AddMaterial(
		const std::string& name,
		std::shared_ptr<MaterialInterface> material)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		EntityId id = GetMaterialNewId();
		id_material_map_.insert({ id, material });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	EntityId Level::AddBuffer(
		const std::string& name,
		std::shared_ptr<BufferInterface> buffer)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		EntityId id = GetBufferNewId();
		id_buffer_map_.insert({ id, buffer });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	EntityId Level::AddStaticMesh(
		const std::string& name,
		std::shared_ptr<StaticMeshInterface> static_mesh)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		EntityId id = GetStaticMeshNewId();
		id_static_mesh_map_.insert({ id, static_mesh });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	const std::vector<frame::EntityId> Level::GetChildList(EntityId id) const
	{
		std::vector<EntityId> list;
		// TODO(anirul): Should probably replace this by a find to throw a more
		// TODO(anirul): explicit error.
		const auto node = id_scene_node_map_.at(id);
		// Check who has node as a parent.
		for (const auto& id : id_scene_node_map_)
		{
			// In case this is node then add it to the list.
			if (id.second->GetParentName() == node->GetName())
			{
				list.push_back(id.first);
			}
		}
		return list;
	}

} // End namespace frame.
