#include "LevelBase.h"

namespace frame {

	void LevelBase::AddSceneTree(
		std::shared_ptr<SceneTreeInterface> scene_tree)
	{
		scene_tree_ = scene_tree;
	}

	std::uint64_t LevelBase::AddTexture(
		const std::string& name,
		std::shared_ptr<TextureInterface> texture)
	{
		std::uint64_t id = GetTextureNewId();
		id_texture_map_.insert({ id, texture });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

	std::uint64_t LevelBase::AddProgram(
		const std::string& name,
		std::shared_ptr<ProgramInterface> program)
	{
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
		std::uint64_t id = GetStaticMeshNewId();
		id_static_mesh_map_.insert({ id, static_mesh });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		return id;
	}

} // End namespace frame.
