#include "Frame/Proto/Test/ParseLevelTest.h"

#include "Frame/Proto/ParseLevel.h"
#include "Frame/Proto/ProtoLevelCreate.h"

namespace test {

	TEST_F(ParseLevelTest, CreateLevelProtoTest)
	{
		EXPECT_TRUE(frame::proto::ParseLevelOpenGL(
			std::make_pair<std::uint32_t, std::uint32_t>(320, 200),
			frame::proto::GetLevel(),
			frame::proto::GetProgramFile(),
			frame::proto::GetSceneFile(),
			frame::proto::GetTextureFile(),
			frame::proto::GetMaterialFile()));
	}

} // End namespace test.
