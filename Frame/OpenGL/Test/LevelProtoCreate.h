#pragma once

#include "Frame/Proto/Proto.h"

namespace test {

	frame::proto::Level GetLevel();
	frame::proto::ProgramFile GetProgramFile();
	frame::proto::SceneTreeFile GetSceneFile();
	frame::proto::TextureFile GetTextureFile();
	frame::proto::MaterialFile GetMaterialFile();

} // End namespace test.
