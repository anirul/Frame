#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "Frame/Error.h"
#include "Frame/LevelInterface.h"
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
			LevelInterface* level,
			EntityId point_buffer_id,
			EntityId normal_buffer_id,
			EntityId texture_buffer_id,
			EntityId index_buffer_id,
			EntityId material_id = -1);
		virtual ~StaticMesh();

	public:
		void SetMaterialId(EntityId id) override
		{
			material_id_ = id;
		}
		EntityId GetMaterialId() const override
		{ 
			return material_id_; 
		}
		EntityId GetPointBufferId() const override
		{ 
			return point_buffer_id_; 
		}
		EntityId GetNormalBufferId() const override 
		{ 
			return normal_buffer_id_; 
		}
		EntityId GetTextureBufferId() const override 
		{ 
			return texture_buffer_id_; 
		}
		EntityId GetIndexBufferId() const  override 
		{ 
			return index_buffer_id_; 
		}
		std::size_t GetIndexSize() const override
		{
			return index_size_;
		}
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }
		unsigned int GetId() const override { return vertex_array_object_; }
		bool IsClearBuffer() const override { return clear_depth_buffer_; }

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;

	protected:
		bool clear_depth_buffer_ = false;
		mutable bool locked_bind_ = false;
		EntityId material_id_ = 0;
		EntityId point_buffer_id_ = 0;
		EntityId normal_buffer_id_ = 0;
		EntityId texture_buffer_id_ = 0;
		EntityId index_buffer_id_ = 0;
		size_t index_size_ = 0;
		unsigned int vertex_array_object_ = 0;
		const Error& error_ = Error::GetInstance();
		std::string material_name_ = "";
	};

	// Create a quad static mesh.
	EntityId CreateQuadStaticMesh(LevelInterface* level);
	// Create a cube static mesh.
	EntityId CreateCubeStaticMesh(LevelInterface* level);

} // End namespace frame::opengl.
