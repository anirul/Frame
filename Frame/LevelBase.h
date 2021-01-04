#pragma once

#include <cinttypes>
#include <memory>
#include <utility>
#include "../Frame/LevelInterface.h"
#include "../Frame/Proto/Proto.h"

namespace frame {

	class LevelBase : public LevelInterface
	{
	protected:
		LevelBase() = default;

	public:
		const std::shared_ptr<SceneTreeInterface> GetSceneTree() const override
		{
			return scene_tree_;
		}
		const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<TextureInterface>>&
			GetTextureMap() const override
		{
			return id_texture_map_;
		}
		const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<ProgramInterface>>&
			GetProgramMap() const override
		{
			return id_program_map_;
		}
		const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<MaterialInterface>>&
			GetMaterialMap() const override
		{
			return id_material_map_;
		}
		const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<BufferInterface>>&
			GetBufferMap() const override
		{
			return id_buffer_map_;
		}
		const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<StaticMeshInterface>>&
			GetStaticMeshMap() const override
		{
			return id_static_mesh_map_;
		}
		std::uint64_t GetIdFromName(const std::string& name) const override
		{
			return name_id_map_.at(name);
		}
		std::string GetNameFromId(std::uint64_t id) const override
		{
			return id_name_map_.at(id);
		}
		std::uint64_t GetDefaultOutputTextureId() const override
		{
			return GetIdFromName(default_texture_name_);
		}
		std::uint64_t GetDefaultScneeId() const override
		{
			return GetIdFromName(default_scene_name_);
		}

	protected:
		std::uint64_t GetTextureNewId() const
		{
			return next_id_maker_++;
		}
		std::uint64_t GetProgramNewId() const
		{
			return next_id_maker_++;
		}
		std::uint64_t GetMaterialNewId() const
		{
			return next_id_maker_++;
		}
		std::uint64_t GetBufferNewId() const
		{
			return next_id_maker_++;
		}
		std::uint64_t GetStaticMeshNewId() const
		{
			return next_id_maker_++;
		}

	protected:
		mutable std::uint64_t next_id_maker_ = 1;
		std::string name_ = "";
		std::string default_texture_name_ = "";
		std::string default_scene_name_ = "";
		std::string default_camera_name_ = "";
		std::shared_ptr<SceneTreeInterface> scene_tree_ = nullptr;
		std::unordered_map<std::uint64_t, std::shared_ptr<TextureInterface>>
			id_texture_map_ = {};
		std::unordered_map<std::uint64_t, std::shared_ptr<ProgramInterface>>
			id_program_map_ = {};
		std::unordered_map<std::uint64_t, std::shared_ptr<MaterialInterface>>
			id_material_map_ = {};
		std::unordered_map<std::uint64_t, std::shared_ptr<BufferInterface>>
			id_buffer_map_ = {};
		std::unordered_map<std::uint64_t, std::shared_ptr<StaticMeshInterface>>
			id_static_mesh_map_ = {};
		std::unordered_map<std::string, std::uint64_t> name_id_map_ = {};
		std::unordered_map<std::uint64_t, std::string> id_name_map_ = {};
	};

} // End namespace frame.
