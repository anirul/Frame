#include "LevelBase.h"

namespace frame {

	std::uint64_t LevelBase::AddSceneNode(
		const std::string& name, 
		std::shared_ptr<SceneNodeInterface> scene_node)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		std::uint64_t id = GetSceneNodeNewId();
		string_set_.insert(name);
		id_scene_node_map_.insert({ id, scene_node });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	std::uint64_t LevelBase::AddTexture(
		const std::string& name,
		std::shared_ptr<TextureInterface> texture)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		std::uint64_t id = GetTextureNewId();
		string_set_.insert(name);
		id_texture_map_.insert({ id, texture });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	std::uint64_t LevelBase::AddProgram(
		const std::string& name,
		std::shared_ptr<ProgramInterface> program)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		std::uint64_t id = GetProgramNewId();
		id_program_map_.insert({ id, program });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	std::uint64_t LevelBase::AddMaterial(
		const std::string& name,
		std::shared_ptr<MaterialInterface> material)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		std::uint64_t id = GetMaterialNewId();
		id_material_map_.insert({ id, material });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	std::uint64_t LevelBase::AddBuffer(
		const std::string& name,
		std::shared_ptr<BufferInterface> buffer)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		std::uint64_t id = GetBufferNewId();
		id_buffer_map_.insert({ id, buffer });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	std::uint64_t LevelBase::AddStaticMesh(
		const std::string& name,
		std::shared_ptr<StaticMeshInterface> static_mesh)
	{
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		std::uint64_t id = GetStaticMeshNewId();
		id_static_mesh_map_.insert({ id, static_mesh });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

} // End namespace frame.
