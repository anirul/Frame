#include "MaterialTest.h"

namespace test {

	TEST_F(MaterialTest, CreateMaterialTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(material_);
		material_ = std::make_shared<sgl::Material>();
		EXPECT_TRUE(material_);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(MaterialTest, CopyConstructorTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		auto material1 = sgl::Material();
		auto texture = std::make_shared<sgl::Texture>(
			"../Asset/CubeMap/PositiveX.png");
		material1.AddTexture("PositiveX", texture);
		auto material2 = sgl::Material(material1);
		EXPECT_EQ(1, material2.GetMap().size());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(MaterialTest, CheckAddRemoveTextureTest)
	{
		EXPECT_FALSE(material_);
		material_ = std::make_shared<sgl::Material>();
		EXPECT_TRUE(material_);
		auto texture1 = std::make_shared<sgl::Texture>(
			"../Asset/CubeMap/PositiveX.png");
		auto texture2 = std::make_shared<sgl::Texture>(
			"../Asset/CubeMap/PositiveY.png");
		material_->AddTexture("PositiveX", texture1);
		material_->AddTexture("PositiveY", texture2);
		EXPECT_TRUE(material_->HasTexture("PositiveX"));
		EXPECT_TRUE(material_->HasTexture("PositiveY"));
		EXPECT_FALSE(material_->HasTexture("PositiveZ"));
		EXPECT_FALSE(material_->RemoveTexture("PositiveZ"));
		EXPECT_TRUE(material_->RemoveTexture("PositiveX"));
		material_->AddTexture("PositiveX", texture1);
		EXPECT_EQ(2, material_->GetMap().size());
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(MaterialTest, MaterialAdditionTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		auto material1 = sgl::Material();
		auto material2 = sgl::Material();
		auto texture1 = std::make_shared<sgl::Texture>(
			"../Asset/CubeMap/PositiveX.png");
		auto texture2 = std::make_shared<sgl::Texture>(
			"../Asset/CubeMap/PositiveY.png");
		material1.AddTexture("PositiveX", texture1);
		material2.AddTexture("PositiveY", texture2);
		auto material3 = material1 + material2;
		EXPECT_EQ(2, material3.GetMap().size());
		material1 += material1;
		EXPECT_EQ(1, material1.GetMap().size());
		material1 += material2;
		EXPECT_EQ(2, material1.GetMap().size());
		material2 += material3;
		EXPECT_EQ(2, material2.GetMap().size());
		EXPECT_NO_THROW(error_.Display());
	}

} // End namespace test.