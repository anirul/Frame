#pragma once

#include <cstdint>

namespace frame {

	enum class EntityTypeEnum : std::uint8_t 
	{
		UNKNOWN = 0,
		NODE = 1,
		TEXTURE = 2,
		PROGRAM = 3,
		MATERIAL = 4,
		BUFFER = 5,
		STATIC_MESH = 6,
	};

	using EntityId = std::int64_t;

} // End namespace frame.
