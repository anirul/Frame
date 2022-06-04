#include "Frame/Proto/Test/ParseLevelTest.h"

#include "Frame/File/FileSystem.h"
#include "Frame/Proto/ParseLevel.h"

namespace test {

TEST_F(ParseLevelTest, CreateLevelProtoTest) {
    EXPECT_TRUE(
        frame::proto::ParseLevelOpenGL(std::make_pair<std::uint32_t, std::uint32_t>(320, 200),
                                       frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
                                           frame::file::FindFile("Asset/Json/LevelTest.json"))));
}

}  // End namespace test.
