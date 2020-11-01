#include "ConvertTest.h"
#include "UniformMock.h"
#include "ProgramMock.h"

namespace test {

	using ::testing::_;
	using ::testing::AtLeast;
	using ::testing::Exactly;
	using ::testing::Return;
	using ::testing::StrictMock;
	using ::testing::TypedEq;

	TEST_F(ConvertTest, ParseUniformFloatTest)
	{	
		frame::proto::Uniform uniform;
		uniform.set_name("test");
		uniform.set_uniform_float(1.0f);
		EXPECT_EQ(1.0f, sgl::ParseUniform(uniform.uniform_float()));
	}

	TEST_F(ConvertTest, ParseUniformVec2Test)
	{
		frame::proto::UniformVector2 uniform_vec2;
		uniform_vec2.set_x(1.0f);
		uniform_vec2.set_y(2.0f);
		EXPECT_EQ(glm::vec2(1.0f, 2.0f), sgl::ParseUniform(uniform_vec2));
	}

	TEST_F(ConvertTest, ParseUniformVec3Test)
	{
		frame::proto::UniformVector3 uniform_vec3;
		uniform_vec3.set_x(1.0f);
		uniform_vec3.set_y(2.0f);
		uniform_vec3.set_z(3.0f);
		EXPECT_EQ(
			glm::vec3(1.0f, 2.0f, 3.0f), 
			sgl::ParseUniform(uniform_vec3));
	}

	TEST_F(ConvertTest, ParseUniformVec4Test)
	{
		frame::proto::UniformVector4 uniform_vec4;
		uniform_vec4.set_x(1.0f);
		uniform_vec4.set_y(2.0f);
		uniform_vec4.set_z(3.0f);
		uniform_vec4.set_w(4.0f);
		EXPECT_EQ(
			glm::vec4(1.0f, 2.0f, 3.0f, 4.0f), 
			sgl::ParseUniform(uniform_vec4));
	}

	TEST_F(ConvertTest, ParseUniformMat4Test)
	{
		frame::proto::UniformMatrix4 uniform_mat4;
		uniform_mat4.set_m11(1.0f);
		uniform_mat4.set_m22(1.0f);
		uniform_mat4.set_m33(1.0f);
		uniform_mat4.set_m44(1.0f);
		EXPECT_EQ(glm::mat4(1.0f), sgl::ParseUniform(uniform_mat4));
	}

	TEST_F(ConvertTest, ParseUniformVecFloatsTest)
	{
		frame::proto::UniformFloats uniform_floats{};
		uniform_floats.add_values(1.0f);
		uniform_floats.add_values(2.0f);
		uniform_floats.add_values(3.0f);
		std::string name = "test_value";
		StrictMock<ProgramMock> program_mock{};
		{
			::testing::InSequence seq;
			EXPECT_CALL(
				program_mock, 
				Uniform(name + "[0]", TypedEq<float>(1.0f)))
					.Times(Exactly(1));
			EXPECT_CALL(
				program_mock,
				Uniform(name + "[1]", TypedEq<float>(2.0f)))
					.Times(Exactly(1));
			EXPECT_CALL(
				program_mock,
				Uniform(name + "[2]", TypedEq<float>(3.0f)))
					.Times(Exactly(1));
		}
		sgl::ParseUniformVec<float>(
			name, 
			uniform_floats, 
			program_mock);
	}

	TEST_F(ConvertTest, ParseUniformFromProtoEmptyTest)
	{
		frame::proto::Uniform uniform;
		StrictMock<ProgramMock> program_mock{};
		StrictMock<UniformMock> uniform_mock{};
		EXPECT_THROW(
			sgl::RegisterUniformFromProto(uniform, uniform_mock, program_mock), 
			std::runtime_error);
	}

	TEST_F(ConvertTest, ParseUniformFromProtoTest)
	{
		frame::proto::Uniform uniform;
		StrictMock<ProgramMock> program_mock{};
		StrictMock<UniformMock> uniform_mock{};
		{
			EXPECT_CALL(
				program_mock,
				Uniform("test", TypedEq<float>(1.0f)))
					.Times(Exactly(1));
		}
		uniform.set_name("test");
		uniform.set_uniform_float(1.0f);
		sgl::RegisterUniformFromProto(uniform, uniform_mock, program_mock);
	}

	TEST_F(ConvertTest, ParseUniformEnumInvalidFromProtoTest)
	{
		frame::proto::Uniform::UniformEnum uniform_enum = 
			frame::proto::Uniform::INVALID;
		StrictMock<ProgramMock> program_mock{};
		StrictMock<UniformMock> uniform_mock{};
		EXPECT_THROW(
			sgl::RegisterUniformEnumFromProto(
				uniform_enum, 
				uniform_mock, 
				program_mock),
			std::runtime_error);
	}

	void ConvertTest::TestParseUniformEnumMatrixFromProto(
		frame::proto::Uniform::UniformEnum uniform_enum,
		const std::string& name)
	{
		StrictMock<ProgramMock> program_mock{};
		StrictMock<UniformMock> uniform_mock{};
		{
			switch (uniform_enum)
			{
				case frame::proto::Uniform::MODEL_MAT4:
				case frame::proto::Uniform::MODEL_INV_MAT4:
					EXPECT_CALL(uniform_mock, GetModel())
						.Times(Exactly(1))
						.WillOnce(Return(glm::mat4(1.0f)));
					break;
				case frame::proto::Uniform::PROJECTION_MAT4:
				case frame::proto::Uniform::PROJECTION_INV_MAT4:
					EXPECT_CALL(uniform_mock, GetProjection())
						.Times(Exactly(1))
						.WillOnce(Return(glm::mat4(1.0f)));
					break;
				case frame::proto::Uniform::VIEW_MAT4:
				case frame::proto::Uniform::VIEW_INV_MAT4:
					EXPECT_CALL(uniform_mock, GetView())
						.Times(Exactly(1))
						.WillOnce(Return(glm::mat4(1.0f)));
					break;
				default:
					throw std::runtime_error("Invalid call!");
			}
			EXPECT_CALL(
				program_mock,
				Uniform(
					name,
					TypedEq<const glm::mat4&>(glm::mat4(1.0f)),
					false))
				.Times(Exactly(1));
		}
		sgl::RegisterUniformEnumFromProto(
			uniform_enum,
			uniform_mock,
			program_mock);
	}

	TEST_F(ConvertTest, ParseUniformEnumProjectionFromProtoTest)
	{
		TestParseUniformEnumMatrixFromProto(
			frame::proto::Uniform::PROJECTION_MAT4, 
			"projection");
	}

	TEST_F(ConvertTest, ParseUniformEnumProjectionInvFromProtoTest)
	{
		TestParseUniformEnumMatrixFromProto(
			frame::proto::Uniform::PROJECTION_INV_MAT4,
			"projection_inv");
	}

	TEST_F(ConvertTest, ParseUniformEnumViewFromProtoTest)
	{
		TestParseUniformEnumMatrixFromProto(
			frame::proto::Uniform::VIEW_MAT4,
			"view");
	}

	TEST_F(ConvertTest, ParseUniformEnumViewInvFromProtoTest)
	{
		TestParseUniformEnumMatrixFromProto(
			frame::proto::Uniform::VIEW_INV_MAT4,
			"view_inv");
	}

	TEST_F(ConvertTest, ParseUniformEnumModelFromProtoTest)
	{
		TestParseUniformEnumMatrixFromProto(
			frame::proto::Uniform::MODEL_MAT4,
			"model");
	}

	TEST_F(ConvertTest, ParseUniformEnumModelInvFromProtoTest)
	{
		TestParseUniformEnumMatrixFromProto(
			frame::proto::Uniform::MODEL_INV_MAT4,
			"model_inv");
	}

	void ConvertTest::TestParseUniformEnumVectorFromProto(
		frame::proto::Uniform::UniformEnum uniform_enum,
		const glm::vec3& compare_vec3,
		const std::string& name)
	{
		StrictMock<ProgramMock> program_mock{};
		StrictMock<UniformMock> uniform_mock{};
		{
			EXPECT_CALL(uniform_mock, GetCamera())
				.Times(Exactly(1))
				.WillOnce(Return(sgl::Camera{}));
			EXPECT_CALL(
				program_mock,
				Uniform(
					name,
					TypedEq<const glm::vec3&>(compare_vec3)))
				.Times(Exactly(1));
		}
		sgl::RegisterUniformEnumFromProto(
			uniform_enum,
			uniform_mock,
			program_mock);
	}

	TEST_F(ConvertTest, ParseUniformEnumCameraPosFromProtoTest)
	{
		TestParseUniformEnumVectorFromProto(
			frame::proto::Uniform::CAMERA_POSITION_VEC3,
			glm::vec3(0, 0, 0),
			"camera_position");
	}

	TEST_F(ConvertTest, ParseUniformEnumCameraDirFromProtoTest)
	{
		TestParseUniformEnumVectorFromProto(
			frame::proto::Uniform::CAMERA_DIRECTION_VEC3,
			glm::vec3(0, 0, -1),
			"camera_direction");
	}

} // End namespace test.
