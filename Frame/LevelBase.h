#pragma once

#include <cinttypes>
#include <memory>
#include <utility>
#include "Frame/LevelInterface.h"

namespace frame {

	class LevelBase : public LevelInterface
	{
	public:
		LevelBase() = default;

	public:
		EntityId GetDefaultQuadSceneId() const final { return quad_id_; }
		void SetDefaultQuadSceneId(EntityId id) final { quad_id_ = id; }
		EntityId GetDefaultCubeSceneId() const final { return cube_id_; }
		void SetDefaultCubeSceneId(EntityId id) final { cube_id_ = id; }

	public:
		const std::unordered_map<EntityId, std::shared_ptr<NodeInterface>>&
			GetSceneNodeMap() const override
		{
			return id_scene_node_map_;
		}
		const std::unordered_map<EntityId, std::shared_ptr<TextureInterface>>&
			GetTextureMap() const override
		{
			return id_texture_map_;
		}
		const std::unordered_map<EntityId, std::shared_ptr<ProgramInterface>>&
			GetProgramMap() const override
		{
			return id_program_map_;
		}
		const std::unordered_map<EntityId, std::shared_ptr<MaterialInterface>>&
			GetMaterialMap() const override
		{
			return id_material_map_;
		}
		const std::unordered_map<EntityId, std::shared_ptr<BufferInterface>>&
			GetBufferMap() const override
		{
			return id_buffer_map_;
		}
		const std::unordered_map<
			EntityId, 
			std::shared_ptr<StaticMeshInterface>>&
			GetStaticMeshMap() const override
		{
			return id_static_mesh_map_;
		}
		EntityId GetIdFromName(const std::string& name) const override
		{
			return name_id_map_.at(name);
		}
		std::string GetNameFromId(EntityId id) const override
		{
			return id_name_map_.at(id);
		}
		EntityId GetDefaultOutputTextureId() const override
		{
			return GetIdFromName(default_texture_name_);
		}
		void SetDefaultCameraName(const std::string& name) override
		{
			default_camera_name_ = name;
		}
		EntityId GetDefaultRootSceneNodeId() const override
		{
			return GetIdFromName(default_root_scene_node_name_);
		}
		void SetDefaultRootSceneNodeName(const std::string& name) override
		{
			default_root_scene_node_name_ = name;
		}
		EntityId GetDefaultCameraId() const override
		{
			assert(!default_camera_name_.empty());
			return GetIdFromName(default_camera_name_);
		}

	public:
		EntityId AddSceneNode(
			const std::string& name,
			std::shared_ptr<NodeInterface> scene_node) override;
		EntityId AddTexture(
			const std::string& name,
			std::shared_ptr<TextureInterface> texture) override;
		EntityId AddProgram(
			const std::string& name,
			std::shared_ptr<ProgramInterface> program) override;
		EntityId AddMaterial(
			const std::string& name,
			std::shared_ptr<MaterialInterface> material) override;
		EntityId AddBuffer(
			const std::string& name,
			std::shared_ptr<BufferInterface> buffer) override;
		EntityId AddStaticMesh(
			const std::string& name,
			std::shared_ptr<StaticMeshInterface> static_mesh) override;

	protected:
		EntityId GetTextureNewId() const
		{
			return next_id_maker_++;
		}
		EntityId GetProgramNewId() const
		{
			return next_id_maker_++;
		}
		EntityId GetMaterialNewId() const
		{
			return next_id_maker_++;
		}
		EntityId GetBufferNewId() const
		{
			return next_id_maker_++;
		}
		EntityId GetStaticMeshNewId() const
		{
			return next_id_maker_++;
		}
		EntityId GetSceneNodeNewId() const
		{
			return next_id_maker_++;
		}

	protected:
		mutable EntityId next_id_maker_ = 1;
		EntityId quad_id_ = 0;
		EntityId cube_id_ = 0;
		std::string name_ = "";
		std::string default_texture_name_ = "";
		std::string default_root_scene_node_name_ = "";
		std::string default_camera_name_ = "";
		std::unordered_set<std::string> string_set_ = {};
		std::unordered_map<EntityId, std::shared_ptr<NodeInterface>>
			id_scene_node_map_ = {};
		std::unordered_map<EntityId, std::shared_ptr<TextureInterface>>
			id_texture_map_ = {};
		std::unordered_map<EntityId, std::shared_ptr<ProgramInterface>>
			id_program_map_ = {};
		std::unordered_map<EntityId, std::shared_ptr<MaterialInterface>>
			id_material_map_ = {};
		std::unordered_map<EntityId, std::shared_ptr<BufferInterface>>
			id_buffer_map_ = {};
		std::unordered_map<EntityId, std::shared_ptr<StaticMeshInterface>>
			id_static_mesh_map_ = {};
		std::unordered_map<std::string, EntityId> name_id_map_ = {};
		std::unordered_map<EntityId, std::string> id_name_map_ = {};
	};

} // End namespace frame.
