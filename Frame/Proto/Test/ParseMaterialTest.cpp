#include "Frame/Proto/Test/ParseMaterialTest.h"

#include "Frame/Proto/ParseMaterial.h"
#include "Frame/Proto/ProtoLevelCreate.h"

namespace test {

	TEST_F(ParseMaterialTest, CreateParseMaterialTest)
	{
		frame::proto::Material proto_material{};
		proto_material.set_name("material_test");
		proto_material.add_texture_names("texture");
		proto_material.add_inner_names("Texture");
		EXPECT_TRUE(frame::proto::ParseMaterialOpenGL(
			proto_material, 
			level_.get()));
	}

} // End namespace test.
