#pragma once

#include <memory>
#include <unordered_map>

namespace frame {

	struct SceneTreeInterface;
	struct TextureInterface;
	struct ProgramInterface;
	struct MaterialInterface;
	struct BufferInterface;
	struct StaticMeshInterface;

	struct LevelInterface
	{
		virtual const std::shared_ptr<SceneTreeInterface>
			GetSceneTree() const = 0;
		virtual const
			std::unordered_map<
				std::uint64_t, 
				std::shared_ptr<TextureInterface>>&
			GetTextureMap() const = 0;
		virtual const
			std::unordered_map<
				std::uint64_t,
				std::shared_ptr<ProgramInterface>>&
			GetProgramMap() const = 0;
		virtual const
			std::unordered_map<
				std::uint64_t,
				std::shared_ptr<MaterialInterface>>&
			GetMaterialMap() const = 0;
		virtual const
			std::unordered_map<
				std::uint64_t,
				std::shared_ptr<BufferInterface>>&
			GetBufferMap() const = 0;
		virtual const
			std::unordered_map<
				std::uint64_t,
				std::shared_ptr<StaticMeshInterface>>&
			GetStaticMeshMap() const = 0;
		virtual std::uint64_t GetIdFromName(const std::string& name) const = 0;
		virtual std::string GetNameFromId(const std::uint64_t id) const = 0;
		virtual std::uint64_t GetDefaultOutputTextureId() const = 0;
		virtual std::uint64_t GetDefaultScneeId() const = 0;
	};

} // End namespace frame.
