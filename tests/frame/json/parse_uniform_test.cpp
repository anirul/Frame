#include "frame/json/parse_uniform_test.h"

#include "frame/json/parse_uniform.h"

namespace test
{

TEST_F(ParseUniformTest, ParseUniformSimpleTest)
{
    bool b = frame::proto::ParseUniform(true);
    EXPECT_EQ(true, b);
    int i = frame::proto::ParseUniform(1);
    EXPECT_EQ(1, i);
    float f = frame::proto::ParseUniform(0.1f);
    EXPECT_FLOAT_EQ(0.1f, f);
}

TEST_F(ParseUniformTest, ParseUniformVector2Test)
{
    frame::proto::UniformVector2 uniform_vector2{};
    uniform_vector2.set_x(0.1f);
    uniform_vector2.set_y(0.2f);
    glm::vec2 vec2 = frame::proto::ParseUniform(uniform_vector2);
    EXPECT_FLOAT_EQ(0.1f, vec2.x);
    EXPECT_FLOAT_EQ(0.2f, vec2.y);
}

TEST_F(ParseUniformTest, ParseUniformVector3Test)
{
    frame::proto::UniformVector3 uniform_vector3{};
    uniform_vector3.set_x(0.1f);
    uniform_vector3.set_y(0.2f);
    uniform_vector3.set_z(0.3f);
    glm::vec3 vec3 = frame::proto::ParseUniform(uniform_vector3);
    EXPECT_FLOAT_EQ(0.1f, vec3.x);
    EXPECT_FLOAT_EQ(0.2f, vec3.y);
    EXPECT_FLOAT_EQ(0.3f, vec3.z);
}

TEST_F(ParseUniformTest, ParseUniformVector4Test)
{
    frame::proto::UniformVector4 uniform_vector4{};
    uniform_vector4.set_x(0.1f);
    uniform_vector4.set_y(0.2f);
    uniform_vector4.set_z(0.3f);
    uniform_vector4.set_w(0.4f);
    glm::vec4 vec4 = frame::proto::ParseUniform(uniform_vector4);
    EXPECT_FLOAT_EQ(0.1f, vec4.x);
    EXPECT_FLOAT_EQ(0.2f, vec4.y);
    EXPECT_FLOAT_EQ(0.3f, vec4.z);
    EXPECT_FLOAT_EQ(0.4f, vec4.w);
}

TEST_F(ParseUniformTest, ParseUniformMat4Test)
{
    frame::proto::UniformMatrix4 uniform_matrix4{};

    uniform_matrix4.set_m11(0.11f);
    uniform_matrix4.set_m12(0.12f);
    uniform_matrix4.set_m13(0.13f);
    uniform_matrix4.set_m14(0.14f);

    uniform_matrix4.set_m21(0.21f);
    uniform_matrix4.set_m22(0.22f);
    uniform_matrix4.set_m23(0.23f);
    uniform_matrix4.set_m24(0.24f);

    uniform_matrix4.set_m31(0.31f);
    uniform_matrix4.set_m32(0.32f);
    uniform_matrix4.set_m33(0.33f);
    uniform_matrix4.set_m34(0.34f);

    uniform_matrix4.set_m41(0.41f);
    uniform_matrix4.set_m42(0.42f);
    uniform_matrix4.set_m43(0.43f);
    uniform_matrix4.set_m44(0.44f);

    glm::mat4 mat4 = frame::proto::ParseUniform(uniform_matrix4);

    EXPECT_FLOAT_EQ(0.11f, mat4[0][0]);
    EXPECT_FLOAT_EQ(0.12f, mat4[0][1]);
    EXPECT_FLOAT_EQ(0.13f, mat4[0][2]);
    EXPECT_FLOAT_EQ(0.14f, mat4[0][3]);

    EXPECT_FLOAT_EQ(0.21f, mat4[1][0]);
    EXPECT_FLOAT_EQ(0.22f, mat4[1][1]);
    EXPECT_FLOAT_EQ(0.23f, mat4[1][2]);
    EXPECT_FLOAT_EQ(0.24f, mat4[1][3]);

    EXPECT_FLOAT_EQ(0.31f, mat4[2][0]);
    EXPECT_FLOAT_EQ(0.32f, mat4[2][1]);
    EXPECT_FLOAT_EQ(0.33f, mat4[2][2]);
    EXPECT_FLOAT_EQ(0.34f, mat4[2][3]);

    EXPECT_FLOAT_EQ(0.41f, mat4[3][0]);
    EXPECT_FLOAT_EQ(0.42f, mat4[3][1]);
    EXPECT_FLOAT_EQ(0.43f, mat4[3][2]);
    EXPECT_FLOAT_EQ(0.44f, mat4[3][3]);
}

TEST_F(ParseUniformTest, ParseUniformQuaternionTest)
{
    frame::proto::UniformQuaternion uniform_quaternion{};
    uniform_quaternion.set_x(0.1f);
    uniform_quaternion.set_y(0.2f);
    uniform_quaternion.set_z(0.3f);
    uniform_quaternion.set_w(0.4f);
    glm::quat quat = frame::proto::ParseUniform(uniform_quaternion);
    EXPECT_FLOAT_EQ(0.1f, quat.x);
    EXPECT_FLOAT_EQ(0.2f, quat.y);
    EXPECT_FLOAT_EQ(0.3f, quat.z);
    EXPECT_FLOAT_EQ(0.4f, quat.w);
}

// TODO(anirul): Add test for the remaining functions.

} // End namespace test.
