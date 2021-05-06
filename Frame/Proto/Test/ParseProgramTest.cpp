#include "Frame/Proto/Test/ParseProgramTest.h"

#include "Frame/Proto/ParseProgram.h"

namespace test {

	TEST_F(ParseProgramTest, CreateParseProgramTest)
	{
		const auto proto_program_file = frame::proto::GetProgramFile();
		const auto proto_program = frame::proto::GetProgramFile().programs(0);
		program_ = frame::proto::ParseProgramOpenGL(
			proto_program,
			level_.get());
		EXPECT_TRUE(program_);
	}

} // End namespace test.
