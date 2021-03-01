#pragma once

#include <memory>
#include <unordered_map>
#include "Frame/EntityId.h"
#include "Frame/BufferInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/NodeInterface.h"
#include "Frame/TextureInterface.h"
#include "Frame/ProgramInterface.h"
#include "Frame/MaterialInterface.h"

namespace frame {

	struct LevelInterface
	{
		// Get maps from the store.
		virtual const std::unordered_map<
			EntityId,
			std::shared_ptr<NodeInterface>>&
			GetSceneNodeMap() const = 0;
		virtual const std::unordered_map<
			EntityId,
			std::shared_ptr<TextureInterface>>&
			GetTextureMap() const = 0;
		virtual const std::unordered_map<
			EntityId,
			std::shared_ptr<ProgramInterface>>&
			GetProgramMap() const = 0;
		virtual const std::unordered_map<
			EntityId,
			std::shared_ptr<MaterialInterface>>&
			GetMaterialMap() const = 0;
		virtual const std::unordered_map<
			EntityId,
			std::shared_ptr<BufferInterface>>&
			GetBufferMap() const = 0;
		virtual const std::unordered_map<
			EntityId,
			std::shared_ptr<StaticMeshInterface>>&
			GetStaticMeshMap() const = 0;
		// Get element id and name from the store.
		virtual EntityId GetIdFromName(const std::string& name) const = 0;
		virtual std::string GetNameFromId(const EntityId id) const = 0;
		virtual EntityId GetDefaultOutputTextureId() const = 0;
		virtual void SetDefaultRootSceneNodeName(const std::string& name) = 0;
		virtual EntityId GetDefaultRootSceneNodeId() const = 0;
		virtual void SetDefaultCameraName(const std::string& name) = 0;
		virtual EntityId GetDefaultCameraId() const = 0;
		// Get & Set the default quad and cube.
		virtual EntityId GetDefaultStaticMeshQuadId() const = 0;
		virtual void SetDefaultStaticMeshQuadId(EntityId id) = 0;
		virtual EntityId GetDefaultStaticMeshCubeId() const = 0;
		virtual void SetDefaultStaticMeshCubeId(EntityId id) = 0;
		// Add element to the store.
		virtual EntityId AddSceneNode(
			const std::string& name,
			std::shared_ptr<NodeInterface> scene_node) = 0;
		virtual EntityId AddTexture(
			const std::string& name,
			std::shared_ptr<TextureInterface> texture) = 0;
		virtual EntityId AddProgram(
			const std::string& name,
			std::shared_ptr<ProgramInterface> program) = 0;
		virtual EntityId AddMaterial(
			const std::string& name,
			std::shared_ptr<MaterialInterface> material) = 0;
		virtual EntityId AddBuffer(
			const std::string& name,
			std::shared_ptr<BufferInterface> buffer) = 0;
		virtual EntityId AddStaticMesh(
			const std::string& name,
			std::shared_ptr<StaticMeshInterface> static_mesh) = 0;
	};

} // End namespace frame.
