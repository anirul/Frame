#include "LoadStaticMeshTest.h"
#include "Frame/Level.h"
#include "Frame/OpenGL/File/LoadStaticMesh.h"

namespace test {

	TEST_F(LoadStaticMeshTest, CreateLoadStaticMeshFromFileTest)
	{
		auto level = std::make_unique<frame::Level>();
		ASSERT_TRUE(level);
		auto mesh_vec = frame::opengl::file::LoadStaticMeshesFromFile(
			level.get(),
			"Asset/Model/Monkey.obj",
			"Monkey");
		EXPECT_EQ(1, mesh_vec.size());
		const auto mesh_id = mesh_vec[0]->GetLocalMesh();
		EXPECT_NE(0, mesh_id);
		auto mesh = level->GetStaticMeshMap().at(mesh_id);
		EXPECT_TRUE(mesh);
		EXPECT_LT(0, mesh->GetIndexSize());
	}

	TEST_F(LoadStaticMeshTest, CreateSceneFromFileTest)
	{
		auto level = std::make_unique<frame::Level>();
		ASSERT_TRUE(level);
		auto mesh_vec = frame::opengl::file::LoadStaticMeshesFromFile(
			level.get(),
			"Asset/Model/Scene.obj",
			"Scene");
		EXPECT_EQ(5, mesh_vec.size());
		for (auto mesh : mesh_vec)
		{
			EXPECT_TRUE(mesh);
			auto mesh_id = mesh->GetLocalMesh();
			auto static_mesh = level->GetStaticMeshMap().at(mesh_id);
			EXPECT_TRUE(static_mesh);
			EXPECT_LT(0, static_mesh->GetIndexSize());
			const auto point_id = static_mesh->GetPointBufferId();
			const auto point_buffer = level->GetBufferMap().at(point_id);
			EXPECT_LT(0, point_buffer->GetSize());
			const auto normal_id = static_mesh->GetNormalBufferId();
			const auto normal_buffer = level->GetBufferMap().at(normal_id);
			EXPECT_LT(0, normal_buffer->GetSize());
			const auto texture_id = static_mesh->GetTextureBufferId();
			const auto texture_buffer = level->GetBufferMap().at(texture_id);
			EXPECT_LT(0, texture_buffer->GetSize());
		}
	}

} // End namespace test.
