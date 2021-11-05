#include "Frame/Proto/Test/ParseProgramTest.h"

#include "Frame/Proto/ParseProgram.h"

namespace test {

	TEST_F(ParseProgramTest, CreateParseProgramTest)
	{
		const auto proto_program_file = frame::proto::GetProgramFile();
		const auto proto_program = frame::proto::GetProgramFile().programs(0);
		auto maybe_program = frame::proto::ParseProgramOpenGL(
			proto_program,
			level_.get());
		EXPECT_TRUE(maybe_program);
		program_ = std::move(maybe_program.value());
		EXPECT_TRUE(program_);
	}

} // End namespace test.
