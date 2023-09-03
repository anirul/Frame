#pragma once

#include <memory>

#include "frame/entity_id.h"
#include "frame/json/proto.h"
#include "frame/name_interface.h"

namespace frame {

	/**
	 * @class Mesh parameter
	 * @brief This class is there to pass entity id of buffer and a config
	 * parameter.
	 */
	struct StaticMeshParameter {
		/**
		 * @brief Buffer of points (this has to be present cannot be generated).
		 */
		EntityId point_buffer_id = NullId;
		//! @brief Buffer of points size (should be 3).
		std::uint32_t point_buffer_size = 3;
		//! @brief Buffer of colors (can be generated as white).
		EntityId color_buffer_id = NullId;
		//! @brief Buffer of colors size (should be 3).
		std::uint32_t color_buffer_size = 3;
		//! @brief Buffer of normal (will be generated as 0, 0, -1).
		EntityId normal_buffer_id = NullId;
		//! @brief Buffer of normal size (should be 3).
		std::uint32_t normal_buffer_size = 3;
		//! @brief Texture coordinates (will be generated as 0.5, 0.5).
		EntityId texture_buffer_id = NullId;
		//! @brief Texture coordinates size (should be 2).
		std::uint32_t texture_buffer_size = 2;
		/**
		 * @brief Index of the points 3 per triangle 2 per line and 1 per point.
		 */
		EntityId index_buffer_id = NullId;
		//! @brief The kind of draw that the mesh is.
		proto::SceneStaticMesh::RenderPrimitiveEnum render_primitive_enum =
			proto::SceneStaticMesh::TRIANGLE;
		//! @brief what kind of buffer will be generated (no point of course).
		enum class StaticMeshParameterEnum {
			GENERATE_COLOR,
			GENERATE_INDEX,
			GENERATE_TEXTURE_COORDINATE,
			GENERATE_NORMAL,
		};
		//! @brief The list of generated buffers.
		std::set<StaticMeshParameterEnum> generate_list = {};
	};

	/**
	 * @class StaticMeshInterface
	 * @brief A static mesh is a mesh that cannot change over time (no skeleton).
	 */
	class StaticMeshInterface : public NameInterface {
	public:
		//! @brief Virtual destructor.
		virtual ~StaticMeshInterface() = default;
		/**
		 * @brief Get point buffer id.
		 * @return Current point buffer id.
		 */
		virtual EntityId GetPointBufferId() const = 0;
		/**
		 * @brief Get color buffer id.
		 * @return Current color buffer id.
		 */
		virtual EntityId GetColorBufferId() const = 0;
		/**
		 * @brief Get normal buffer id.
		 * @return Current normal buffer id.
		 */
		virtual EntityId GetNormalBufferId() const = 0;
		/**
		 * @brief Get texture buffer id.
		 * @return Current texture buffer id.
		 */
		virtual EntityId GetTextureBufferId() const = 0;
		/**
		 * @brief Get index buffer id (triangle based).
		 * @return Current index buffer id.
		 */
		virtual EntityId GetIndexBufferId() const = 0;
		/**
		 * @brief This is the size in bytes! so if you need the element size just
		 * divide this number by the sizeof(std::int32_t).
		 * @return Size of the index buffer in bytes!
		 */
		virtual std::size_t GetIndexSize() const = 0;
		/**
		 * @brief Update the index size for streams.
		 * @param level: The level corresponding to this mesh.
		 */
		virtual void SetIndexSize(std::size_t index_size) = 0;
		/**
		 * @brief Check if depth buffer is cleared or not.
		 * @return Is depth buffer cleared?
		 */
		virtual bool IsClearBuffer() const = 0;
		/**
		 * @brief Set the way a mesh is rendered (point/line/triangle) triangle is the
		 * default.
		 * @param render_enum: The basic shape of the renderer that should be used on
		 * this mesh.
		 */
		virtual void SetRenderPrimitive(
			proto::SceneStaticMesh::RenderPrimitiveEnum render_enum) = 0;
		/**
		 * @brief Get the static mesh render primitive.
		 * @return Get the render primitive.
		 */
		virtual proto::SceneStaticMesh::RenderPrimitiveEnum
		GetRenderPrimitive() const = 0;
	};

}  // End namespace frame.
