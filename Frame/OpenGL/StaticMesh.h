#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "Frame/Error.h"
#include "Frame/OpenGL/Buffer.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/Program.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/StaticMeshInterface.h"

namespace frame::opengl {

	class StaticMesh : public StaticMeshInterface
	{
	public:
		// Create a mesh from a set of vectors.
		StaticMesh(
			const std::shared_ptr<LevelInterface> level,
			std::uint64_t point_buffer_id,
			std::uint64_t normal_buffer_id,
			std::uint64_t texture_buffer_id,
			std::uint64_t index_buffer_id);
		virtual ~StaticMesh();

	public:
		void SetMaterialId(std::uint64_t id) override
		{ 
			material_id_ = id; 
		}
		std::uint64_t GetMaterialId() const override
		{ 
			return material_id_; 
		}
		std::uint64_t GetPointBufferId() const override
		{ 
			return point_buffer_id_; 
		}
		std::uint64_t GetNormalBufferId() const override 
		{ 
			return normal_buffer_id_; 
		}
		std::uint64_t GetTextureBufferId() const override 
		{ 
			return texture_buffer_id_; 
		}
		std::uint64_t GetIndexBufferId() const  override 
		{ 
			return index_buffer_id_; 
		}
		std::size_t GetIndexSize() const override
		{
			return level_->GetBufferMap().at(index_buffer_id_)->GetSize();
		}
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }
		unsigned int GetId() const override { return vertex_array_object_; }

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;

	protected:
		bool clear_depth_buffer_ = false;
		mutable bool locked_bind_ = false;
		std::shared_ptr<LevelInterface> level_ = nullptr;
		std::shared_ptr<ProgramInterface> program_ = nullptr;
		std::uint64_t material_id_ = 0;
		std::uint64_t point_buffer_id_ = 0;
		std::uint64_t normal_buffer_id_ = 0;
		std::uint64_t texture_buffer_id_ = 0;
		std::uint64_t index_buffer_id_ = 0;
		std::shared_ptr<MaterialInterface> material_ = nullptr;
		size_t index_size_ = 0;
		unsigned int vertex_array_object_ = 0;
		const Error& error_ = Error::GetInstance();
		std::string material_name_ = "";
	};

	// Create a quad static mesh.
	std::shared_ptr<StaticMeshInterface> CreateQuadStaticMesh(
		std::shared_ptr<LevelInterface> level);
	// Create a cube static mesh.
	std::shared_ptr<StaticMeshInterface> CreateCubeStaticMesh(
		std::shared_ptr<LevelInterface> level);

} // End namespace frame::opengl.
