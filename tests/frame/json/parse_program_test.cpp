#include "frame/json/parse_program_test.h"

#include "frame/json/parse_program.h"

namespace test {

TEST_F(ParseProgramTest, CreateParseProgramTest) {
    const auto proto_program = proto_level_.programs(0);
    auto program       = frame::proto::ParseProgramOpenGL(proto_program, *level_.get());
    EXPECT_TRUE(program);
}

TEST_F(ParseProgramTest, CreateParseProgramUniformTest) {
    const auto proto_program = proto_level_.programs(0);
    auto program       = frame::proto::ParseProgramOpenGL(proto_program, *level_.get());
    EXPECT_TRUE(program);
    program_ = std::move(program);
    const auto uniform_list = program_->GetUniformNameList();
    EXPECT_EQ(2, uniform_list.size());
    EXPECT_EQ(1, std::count(uniform_list.begin(), uniform_list.end(), "exponent"));
    EXPECT_TRUE(program_);
}

}  // End namespace test.
