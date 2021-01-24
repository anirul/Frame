#pragma once

#include <cinttypes>
#include <memory>
#include <utility>
#include "Frame/LevelInterface.h"
#include "Frame/Proto/Proto.h"
#include "Frame/SceneNodeInterface.h"

namespace frame {

	class LevelBase : public LevelInterface
	{
	public:
		LevelBase() = default;

	public:
		const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<SceneNodeInterface>>&
			GetSceneNodeMap() const override
		{
			return id_scene_node_map_;
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
		void SetDefaultCameraName(const std::string& name) override
		{
			default_camera_name_ = name;
		}
		std::uint64_t GetDefaultRootSceneNodeId() const override
		{
			return GetIdFromName(default_root_scene_node_name_);
		}
		void SetDefaultRootSceneNodeName(const std::string& name) override
		{
			default_root_scene_node_name_ = name;
		}
		std::uint64_t GetDefaultCameraId() const override
		{
			assert(!default_camera_name_.empty());
			return GetIdFromName(default_camera_name_);
		}

	public:
		std::uint64_t AddSceneNode(
			const std::string& name,
			std::shared_ptr<SceneNodeInterface> scene_node) override;
		std::uint64_t AddTexture(
			const std::string& name,
			std::shared_ptr<TextureInterface> texture) override;
		std::uint64_t AddProgram(
			const std::string& name,
			std::shared_ptr<ProgramInterface> program) override;
		std::uint64_t AddMaterial(
			const std::string& name,
			std::shared_ptr<MaterialInterface> material) override;
		std::uint64_t AddBuffer(
			const std::string& name,
			std::shared_ptr<BufferInterface> buffer) override;
		std::uint64_t AddStaticMesh(
			const std::string& name,
			std::shared_ptr<StaticMeshInterface> static_mesh) override;

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
		std::uint64_t GetSceneNodeNewId() const
		{
			return next_id_maker_++;
		}

	protected:
		mutable std::uint64_t next_id_maker_ = 1;
		std::string name_ = "";
		std::string default_texture_name_ = "";
		std::string default_root_scene_node_name_ = "";
		std::string default_camera_name_ = "";
		std::unordered_set<std::string> string_set_ = {};
		std::unordered_map<std::uint64_t, std::shared_ptr<SceneNodeInterface>>
			id_scene_node_map_ = {};
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
