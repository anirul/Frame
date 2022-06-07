#include "frame/proto/parse_program_test.h"

#include "frame/proto/parse_program.h"

namespace test {

TEST_F(ParseProgramTest, CreateParseProgramTest) {
    const auto proto_program = proto_level_.programs(0);
    auto maybe_program       = frame::proto::ParseProgramOpenGL(proto_program, level_.get());
    EXPECT_TRUE(maybe_program);
    program_ = std::move(maybe_program.value());
    EXPECT_TRUE(program_);
}

}  // End namespace test.
