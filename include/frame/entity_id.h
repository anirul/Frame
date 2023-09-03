#pragma once

#include <cstdint>

namespace frame {

	/**
	 * @brief The description of what object is on the other side of the entity
	 *        id.
	 */
	enum class EntityTypeEnum : std::uint8_t {
		UNKNOWN = 0,
		NODE = 1,
		TEXTURE = 2,
		PROGRAM = 3,
		MATERIAL = 4,
		BUFFER = 5,
		STATIC_MESH = 6,
		PLUGIN = 7,
	};

	/**
	 * @brief This is used by the code to have an abstraction from pointer to
	 *        structure see data based programming. All object are represented
	 *        by Id that are stored in the level structure.
	 */
	using EntityId = std::int64_t;
	/**
	 * @brief This is an error in case you have a null id this mean there is
	 *        nothing there or an error.
	 */
	constexpr EntityId NullId = 0;

}  // End namespace frame.
