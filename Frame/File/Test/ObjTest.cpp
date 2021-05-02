#include "ObjTest.h"
#include "Frame/File/FileSystem.h"
#include "Frame/File/Obj.h"

namespace test {

	TEST_F(ObjTest, CreateObjTest)
	{
		ASSERT_FALSE(obj_);
		obj_ = std::make_unique<frame::file::Obj>(
			frame::file::FindFile("Asset/Model/Cube.obj"));
		EXPECT_TRUE(obj_);
	}

	TEST_F(ObjTest, ObjGetMeshesTest)
	{
		ASSERT_FALSE(obj_);
		obj_ = std::make_unique<frame::file::Obj>(
			frame::file::FindFile("Asset/Model/Scene.obj"));
		EXPECT_TRUE(obj_);
		EXPECT_NE(0, obj_->GetMeshes().size());
		for (const auto& element : obj_->GetMeshes())
		{
			EXPECT_NE(0, element.GetVertices().size());
			EXPECT_NE(0, element.GetIndices().size());
		}
	}

	TEST_F(ObjTest, ObjGetMaterialTest)
	{
		ASSERT_FALSE(obj_);
		obj_ = std::make_unique<frame::file::Obj>(
			frame::file::FindFile("Asset/Model/Scene.obj"));
		EXPECT_TRUE(obj_);
		EXPECT_NE(0, obj_->GetMeshes().size());
		for (const auto& element : obj_->GetMeshes())
		{
			auto material_id = element.GetMaterialId();
			EXPECT_LT(material_id, obj_->GetMaterials().size());
			const auto material = obj_->GetMaterials()[material_id];
			if (material.ambient_str.empty())
			{
				EXPECT_NE(0.0f, material.ambient_vec4.x);
				EXPECT_NE(0.0f, material.ambient_vec4.y);
				EXPECT_NE(0.0f, material.ambient_vec4.z);
			}
			else
			{
				EXPECT_LT(4, material.ambient_str.size());
			}
			if (material.metallic_str.empty())
				EXPECT_NE(0.0f, material.metallic_val);
			else
				EXPECT_LT(4, material.metallic_str.size());
			if (material.roughness_str.empty())
				EXPECT_NE(0.0f, material.roughness_val);
			else
				EXPECT_LT(4, material.roughness_str.size());
			EXPECT_FALSE(material.normal_str.empty());
		}
	}

} // End namespace test.
