#pragma once

#include <cinttypes>
#include <memory>
#include <utility>

#include "Frame/Logger.h"
#include "Frame/LevelInterface.h"

namespace frame {

	class Level : public LevelInterface
	{
	public:
		Level() = default;
		void SetDefaultStaticMeshQuadId(EntityId id) final { quad_id_ = id; }
		void SetDefaultStaticMeshCubeId(EntityId id) final { cube_id_ = id; }
		// Get Node from Id.
		NodeInterface* GetSceneNodeFromId(EntityId id) const override
		{
			return id_scene_node_map_.at(id).get();
		}
		TextureInterface* GetTextureFromId(EntityId id) const override
		{
			return id_texture_map_.at(id).get();
		}
		ProgramInterface* GetProgramFromId(EntityId id) const override
		{
			return id_program_map_.at(id).get();
		}
		MaterialInterface* GetMaterialFromId(EntityId id) const override
		{
			return id_material_map_.at(id).get();
		}
		BufferInterface* GetBufferFromId(EntityId id) const override
		{
			return id_buffer_map_.at(id).get();
		}
		StaticMeshInterface* GetStaticMeshFromId(
			EntityId id) const override
		{
			return id_static_mesh_map_.at(id).get();
		}
		std::optional<EntityId> GetDefaultOutputTextureId() const override
		{
			return GetIdFromName(default_texture_name_);
		}
		void SetDefaultCameraName(const std::string& name) override
		{
			default_camera_name_ = name;
		}
		std::optional<EntityId> GetDefaultRootSceneNodeId() const override
		{
			return GetIdFromName(default_root_scene_node_name_);
		}
		void SetDefaultRootSceneNodeName(const std::string& name) override
		{
			default_root_scene_node_name_ = name;
		}
		std::optional<EntityId> GetDefaultCameraId() const override
		{
			assert(!default_camera_name_.empty());
			return GetIdFromName(default_camera_name_);
		}

	public:
		std::optional<EntityId> GetDefaultStaticMeshQuadId() const final;
		std::optional<EntityId> GetDefaultStaticMeshCubeId() const final;
		std::optional<EntityId> GetIdFromName(
			const std::string& name) const override;
		std::optional<std::string> GetNameFromId(EntityId id) const override;
		std::optional<EntityId> AddSceneNode(
			std::unique_ptr<NodeInterface>&& scene_node) override;
		std::optional<EntityId> AddTexture(
			std::unique_ptr<TextureInterface>&& texture) override;
		std::optional<EntityId> AddProgram(
			std::unique_ptr<ProgramInterface>&& program) override;
		std::optional<EntityId> AddMaterial(
			std::unique_ptr<MaterialInterface>&& material) override;
		std::optional<EntityId> AddBuffer(
			std::unique_ptr<BufferInterface>&& buffer) override;
		std::optional<EntityId> AddStaticMesh(
			std::unique_ptr<StaticMeshInterface>&& static_mesh) override;
		std::optional<std::vector<EntityId>> GetChildList(
			EntityId id) const override;
		std::optional<EntityId> GetParentId(EntityId id) const override;
		std::unique_ptr<TextureInterface> ExtractTexture(EntityId id) override;
		CameraInterface* GetDefaultCamera() override;

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
		Logger& logger_ = Logger::GetInstance();
		mutable EntityId next_id_maker_ = 1;
		EntityId quad_id_ = 0;
		EntityId cube_id_ = 0;
		std::string name_;
		std::string default_texture_name_;
		std::string default_root_scene_node_name_;
		std::string default_camera_name_;
		std::unordered_set<std::string> string_set_ = {};
		std::unordered_map<EntityId, std::unique_ptr<NodeInterface>>
			id_scene_node_map_ = {};
		std::unordered_map<EntityId, std::unique_ptr<TextureInterface>>
			id_texture_map_ = {};
		std::unordered_map<EntityId, std::unique_ptr<ProgramInterface>>
			id_program_map_ = {};
		std::unordered_map<EntityId, std::unique_ptr<MaterialInterface>>
			id_material_map_ = {};
		std::unordered_map<EntityId, std::unique_ptr<BufferInterface>>
			id_buffer_map_ = {};
		std::unordered_map<EntityId, std::unique_ptr<StaticMeshInterface>>
			id_static_mesh_map_ = {};
		std::unordered_map<std::string, EntityId> name_id_map_ = {};
		std::unordered_map<EntityId, std::string> id_name_map_ = {};
		std::unordered_map<EntityId, EntityTypeEnum> id_enum_map_ = {};
	};

} // End namespace frame.
