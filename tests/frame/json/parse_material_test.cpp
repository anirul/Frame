#include "frame/json/parse_material_test.h"

#include "frame/opengl/json/parse_material.h"

namespace test
{

TEST_F(ParseMaterialTest, CreateParseMaterialTest)
{
    frame::proto::Material proto_material{};
    proto_material.set_name("material_test");
    proto_material.add_buffer_names("triangle_buffer");
    proto_material.add_inner_buffer_names("TriangleBuffer");
    proto_material.add_node_names("DragonMesh");
    proto_material.add_inner_node_names("model");

    material_ = frame::json::ParseMaterialOpenGL(proto_material, *level_);
    ASSERT_TRUE(material_);
    EXPECT_EQ(material_->GetBufferNames().size(), 1u);
    EXPECT_EQ(
        material_->GetInnerBufferName("triangle_buffer"),
        "TriangleBuffer");
    EXPECT_EQ(material_->GetNodeNames().size(), 1u);
    EXPECT_EQ(material_->GetInnerNodeName("DragonMesh"), "model");
}

} // End namespace test.
