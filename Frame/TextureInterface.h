#pragma once

#include "../Frame/BindInterface.h"

namespace frame {

	// Interface to a texture.
	struct TextureInterface : public BindInterface 
	{
		virtual const unsigned int GetId() const = 0;
	};

} // End namespace frame.
