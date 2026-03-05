#include "frame/json/parse_material_test.h"

#include "frame/opengl/json/parse_material.h"

namespace test
{

TEST_F(ParseMaterialTest, CreateParseMaterialTest)
{
    frame::proto::Material proto_material{};
    proto_material.set_name("material_test");
    EXPECT_TRUE(
        frame::json::ParseMaterialOpenGL(proto_material, *level_.get()));
}

} // End namespace test.
