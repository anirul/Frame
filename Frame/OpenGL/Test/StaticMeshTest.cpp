#include "StaticMeshTest.h"
#include <GL/glew.h>
#include "Frame/BufferInterface.h"
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/File/LoadStaticMesh.h"
#include "Frame/Level.h"

namespace test {

	TEST_F(StaticMeshTest, CreateCubeMeshTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(static_mesh_);
		EXPECT_TRUE(window_);
		auto level = std::make_shared<frame::Level>();
		const auto mesh_vec = frame::opengl::file::LoadStaticMeshesFromFile(
			level.get(),
			frame::file::FindFile("Asset/Model/Cube.obj"),
			"cube");
		auto static_mesh_id = mesh_vec[0]->GetLocalMesh();
		EXPECT_NE(0, static_mesh_id);
		static_mesh_ = level->GetStaticMeshMap().at(static_mesh_id);
		EXPECT_EQ(1, mesh_vec.size());
		EXPECT_TRUE(static_mesh_);
		EXPECT_EQ(0, static_mesh_->GetMaterialId());
		EXPECT_NE(0, static_mesh_->GetPointBufferId());
		EXPECT_NE(0, static_mesh_->GetNormalBufferId());
		EXPECT_NE(0, static_mesh_->GetTextureBufferId());
		EXPECT_NE(0, static_mesh_->GetIndexBufferId());
		auto id = static_mesh_->GetIndexBufferId();
		auto index_buffer = level->GetBufferMap().at(id);
		EXPECT_LE(18, index_buffer->GetSize());
		EXPECT_GE(144, index_buffer->GetSize());
		EXPECT_TRUE(static_mesh_);
	}

	TEST_F(StaticMeshTest, CreateTorusMeshTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(static_mesh_);
		EXPECT_TRUE(window_);
		auto level = std::make_shared<frame::Level>();
		const auto mesh_vec = frame::opengl::file::LoadStaticMeshesFromFile(
			level.get(),
			frame::file::FindFile("Asset/Model/Torus.obj"),
			"torus");
		auto static_mesh_id = mesh_vec[0]->GetLocalMesh();
		EXPECT_NE(0, static_mesh_id);
		static_mesh_ = level->GetStaticMeshMap().at(static_mesh_id);
		EXPECT_EQ(1, mesh_vec.size());
		EXPECT_TRUE(static_mesh_);
		EXPECT_EQ(0, static_mesh_->GetMaterialId());
		EXPECT_NE(0, static_mesh_->GetPointBufferId());
		EXPECT_NE(0, static_mesh_->GetNormalBufferId());
		EXPECT_NE(0, static_mesh_->GetTextureBufferId());
		EXPECT_NE(0, static_mesh_->GetIndexBufferId());
		auto id = static_mesh_->GetIndexBufferId();
		auto index_buffer = level->GetBufferMap().at(id);
		EXPECT_LE(3456, index_buffer->GetSize());
		EXPECT_GE(13824, index_buffer->GetSize());
		EXPECT_TRUE(static_mesh_);
	}

} // End namespace test.
