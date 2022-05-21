#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "Frame/CameraInterface.h"
#include "Frame/EntityId.h"
#include "Frame/BufferInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/NodeInterface.h"
#include "Frame/TextureInterface.h"
#include "Frame/ProgramInterface.h"
#include "Frame/MaterialInterface.h"

namespace frame {

	class LevelInterface
	{
	public:
		virtual ~LevelInterface() = default;
		// Get maps from the store.
		virtual NodeInterface* GetSceneNodeFromId(EntityId id) const = 0;
		virtual TextureInterface* GetTextureFromId(EntityId id) const = 0;
		virtual ProgramInterface* GetProgramFromId(EntityId id) const = 0;
		virtual MaterialInterface* GetMaterialFromId(
			EntityId id) const = 0;
		virtual BufferInterface* GetBufferFromId(EntityId id) const = 0;
		virtual StaticMeshInterface* GetStaticMeshFromId(
			EntityId id) const = 0;
		// Get element id and name from the store.
		virtual std::optional<EntityId> GetIdFromName(
			const std::string& name) const = 0;
		virtual std::optional<std::string> GetNameFromId(EntityId id) const = 0;
		virtual std::optional<EntityId> GetDefaultOutputTextureId() const = 0;
		virtual void SetDefaultRootSceneNodeName(const std::string& name) = 0;
		virtual std::optional<EntityId> GetDefaultRootSceneNodeId() const = 0;
		virtual void SetDefaultCameraName(const std::string& name) = 0;
		virtual std::optional<EntityId> GetDefaultCameraId() const = 0;
		// Get the list of children from an id in the node list.
		virtual std::optional<std::vector<EntityId>> GetChildList(
			const EntityId id) const = 0;
		virtual std::optional<EntityId> GetParentId(EntityId id) const = 0;
		// Get & Set the default quad and cube.
		virtual std::optional<EntityId> GetDefaultStaticMeshQuadId() const = 0;
		virtual void SetDefaultStaticMeshQuadId(EntityId id) = 0;
		virtual std::optional<EntityId> GetDefaultStaticMeshCubeId() const = 0;
		virtual void SetDefaultStaticMeshCubeId(EntityId id) = 0;
		// Add element to the store.
		virtual std::optional<EntityId> AddSceneNode(
			std::unique_ptr<NodeInterface>&& scene_node) = 0;
		virtual std::optional<EntityId> AddTexture(
			std::unique_ptr<TextureInterface>&& texture) = 0;
		virtual std::optional<EntityId> AddProgram(
			std::unique_ptr<ProgramInterface>&& program) = 0;
		virtual std::optional<EntityId> AddMaterial(
			std::unique_ptr<MaterialInterface>&& material) = 0;
		virtual std::optional<EntityId> AddBuffer(
			std::unique_ptr<BufferInterface>&& buffer) = 0;
		virtual std::optional<EntityId> AddStaticMesh(
			std::unique_ptr<StaticMeshInterface>&& static_mesh) = 0;
		// Extract an entity from a level (this entity will be unvalidated!).
		virtual std::unique_ptr<TextureInterface> ExtractTexture(
			EntityId id) = 0;
		virtual CameraInterface* GetDefaultCamera() = 0;
		// Interface for 'pre-' and 'post-' processing materials.
		virtual void PushBackPreProcess(
			const std::string& name, 
			const std::string& mesh_name,
			const std::string& material_name) = 0;
        virtual void PushBackPostProcess(
            const std::string& name,
			const std::string& mesh_name,
            const std::string& material_name) = 0;
		virtual const std::vector<std::string>& GetPreProcessNames() const = 0;
        virtual const std::vector<std::string>& GetPostProcessNames() const = 0;
		virtual const std::vector<EntityId>& GetPreProcessMeshIds() const = 0;
        virtual const std::vector<EntityId>& GetPostProcessMeshIds() const = 0;
		virtual const std::vector<EntityId>& 
			GetPreProcessMaterialIds() const = 0;
		virtual const std::vector<EntityId>&
			GetPostProcessMaterialIds() const = 0;
	};

} // End namespace frame.
