#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Convert.h"

namespace test {

	class ConvertTest : public testing::Test
	{
	public:
		ConvertTest() = default;

	protected:
		void TestParseUniformEnumMatrixFromProto(
			frame::proto::Uniform::UniformEnum uniform_enum,
			const std::string& name);
		void TestParseUniformEnumVectorFromProto(
			frame::proto::Uniform::UniformEnum uniform_enum,
			const glm::vec3& compare_vec3,
			const std::string& name);
	};

} // End namespace test.
