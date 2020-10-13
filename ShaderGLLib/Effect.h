#pragma once

#include "../FrameProto/Proto.h"

namespace sgl {

	class Effect {
	public:
		// You will be able to add stuff in the constructor of the derived
		// class. Some thing like mesh, program, mipmap, textures and also
		// functions.
		Effect(const frame::proto::Effect& effect_proto);

	public:
		// Startup the Effect this is where the effect is created (this will be
		// called only once at the beginning).
		void Startup(std::pair<std::uint32_t, std::uint32_t> size);
		// This is the draw interfaces.
		void Draw(const double dt = 0.0);
		// Free everything.
		void Delete();
		// Get the name of the effect.
		const std::string GetName() const;
	};

}