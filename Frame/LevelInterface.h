#pragma once

#include <memory>
#include <unordered_map>
#include "Frame/ProgramInterface.h"
#include "Frame/MaterialInterface.h"
#include "Frame/BufferInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/SceneNodeInterface.h"
#include "Frame/TextureInterface.h"

namespace frame {

	struct LevelInterface
	{
		// Get maps from the store.
		virtual const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<SceneNodeInterface>>&
			GetSceneNodeMap() const = 0;
		virtual const std::unordered_map<
			std::uint64_t, 
			std::shared_ptr<TextureInterface>>&
			GetTextureMap() const = 0;
		virtual const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<ProgramInterface>>&
			GetProgramMap() const = 0;
		virtual const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<MaterialInterface>>&
			GetMaterialMap() const = 0;
		virtual const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<BufferInterface>>&
			GetBufferMap() const = 0;
		virtual const std::unordered_map<
			std::uint64_t,
			std::shared_ptr<StaticMeshInterface>>&
			GetStaticMeshMap() const = 0;
		// Get element id and name from the store.
		virtual std::uint64_t GetIdFromName(const std::string& name) const = 0;
		virtual std::string GetNameFromId(const std::uint64_t id) const = 0;
		virtual std::uint64_t GetDefaultOutputTextureId() const = 0;
		virtual void SetDefaultRootSceneNodeName(const std::string& name) = 0;
		virtual std::uint64_t GetDefaultRootSceneNodeId() const = 0;
		virtual void SetDefaultCameraName(const std::string& name) = 0;
		virtual std::uint64_t GetDefaultCameraId() const = 0;
		// Add element to the store.
		virtual std::uint64_t AddSceneNode(
			const std::string& name,
			std::shared_ptr<SceneNodeInterface> scene_node) = 0;
		virtual std::uint64_t AddTexture(
			const std::string& name,
			std::shared_ptr<TextureInterface> texture) = 0;
		virtual std::uint64_t AddProgram(
			const std::string& name,
			std::shared_ptr<ProgramInterface> program) = 0;
		virtual std::uint64_t AddMaterial(
			const std::string& name,
			std::shared_ptr<MaterialInterface> material) = 0;
		virtual std::uint64_t AddBuffer(
			const std::string& name,
			std::shared_ptr<BufferInterface> buffer) = 0;
		virtual std::uint64_t AddStaticMesh(
			const std::string& name,
			std::shared_ptr<StaticMeshInterface> static_mesh) = 0;
	};

} // End namespace frame.
