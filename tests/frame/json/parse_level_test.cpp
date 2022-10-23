#include "frame/json/parse_level_test.h"

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

namespace test {

TEST_F(ParseLevelTest, CreateLevelProtoTest) {
    EXPECT_TRUE(
        frame::proto::ParseLevel(std::make_pair<std::uint32_t, std::uint32_t>(320, 200),
                                         frame::file::FindFile("asset/json/level_test.json"),
                                         window_->GetUniqueDevice()));
}

}  // End namespace test.
