#include "MaterialTest.h"
#include "Frame/Level.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/File/FileSystem.h"

namespace test {

	TEST_F(MaterialTest, CreateMaterialTest)
	{
		EXPECT_FALSE(material_);
		auto level = std::make_unique<frame::Level>();
		auto material = std::make_unique<frame::opengl::Material>();
		material_ = std::move(material);
		EXPECT_TRUE(material_);
		EXPECT_NO_THROW(error_.Display());
	}

	TEST_F(MaterialTest, CheckAddRemoveTextureTest)
	{
		EXPECT_FALSE(material_);
		auto level = std::make_unique<frame::Level>();
		auto material = std::make_unique<frame::opengl::Material>();
		material_ = std::move(material);
		EXPECT_TRUE(material_);
		auto texture1 = frame::opengl::file::LoadTextureFromFile(
			frame::file::FindDirectory("Asset") + "/CubeMap/PositiveX.png");
		EXPECT_TRUE(texture1);
		texture1->SetName("PositiveX");
        auto texture2 = frame::opengl::file::LoadTextureFromFile(
			frame::file::FindDirectory("Asset") + "/CubeMap/PositiveY.png");
		EXPECT_TRUE(texture2);
		texture2->SetName("PositiveY");
		auto maybe_id1 = level->AddTexture(std::move(texture1));
		EXPECT_TRUE(maybe_id1);
		auto maybe_id2 = level->AddTexture(std::move(texture2));
		EXPECT_TRUE(maybe_id2);
		std::uint64_t id_false = 0;
		auto id1 = maybe_id1.value();
		auto id2 = maybe_id2.value();
		EXPECT_TRUE(material_->AddTextureId(id1, "PositiveX"));
		EXPECT_TRUE(material_->AddTextureId(id2, "PositiveY"));
		EXPECT_TRUE(material_->HasTextureId(id1));
		EXPECT_TRUE(material_->HasTextureId(id2));
		EXPECT_FALSE(id_false);
		EXPECT_FALSE(material_->RemoveTextureId(id_false));
		EXPECT_TRUE(material_->RemoveTextureId(id1));
		material_->AddTextureId(id1, "PositiveX");
		EXPECT_EQ(2, material_->GetIds().size());
		EXPECT_NO_THROW(error_.Display());
	}

} // End namespace test.
