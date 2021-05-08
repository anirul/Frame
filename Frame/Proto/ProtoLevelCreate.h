#pragma once

#include "Frame/Proto/Proto.h"

namespace frame::proto {

	Level GetLevel();
	ProgramFile GetProgramFile();
	SceneTreeFile GetSceneFile();
	TextureFile GetTextureFile(const std::string& filename = "");
	MaterialFile GetMaterialFile();

} // End namespace frame::proto.
