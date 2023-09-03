#include "frame/json/parse_level_test.h"

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

namespace test {

    TEST_F(ParseLevelTest, CreateLevelProtoTest) {
        EXPECT_TRUE(
            frame::proto::ParseLevel(
                glm::uvec2(320, 200),
                frame::file::FindFile("asset/json/level_test.json")));
    }

}  // End namespace test.
