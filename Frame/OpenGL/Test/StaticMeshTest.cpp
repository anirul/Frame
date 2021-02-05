#include "StaticMeshTest.h"
#include <GL/glew.h>
#include "Frame/BufferInterface.h"
#include "Frame/File/FileSystem.h"
#include "Frame/File/LoadStaticMesh.h"
#include "Frame/LevelBase.h"

namespace test {

	TEST_F(StaticMeshTest, CreateCubeMeshTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(static_mesh_);
		EXPECT_TRUE(window_);
		auto level = std::make_shared<frame::LevelBase>();
		static_mesh_ = frame::file::LoadStaticMeshFromFileOpenGL(
			level,
			frame::file::FindPath("Asset") + "/Model/Cube.obj");
		EXPECT_NE(0, static_mesh_->GetMaterialId());
		EXPECT_NE(0, static_mesh_->GetPointBufferId());
		EXPECT_NE(0, static_mesh_->GetNormalBufferId());
		EXPECT_NE(0, static_mesh_->GetTextureBufferId());
		EXPECT_NE(0, static_mesh_->GetIndexBufferId());
		auto id = static_mesh_->GetIndexBufferId();
		auto index_buffer = level->GetBufferMap().at(id);
		EXPECT_LE(18, index_buffer->GetSize());
		EXPECT_GE(36, index_buffer->GetSize());
		EXPECT_TRUE(static_mesh_);
	}

	TEST_F(StaticMeshTest, CreateTorusMeshTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(static_mesh_);
		EXPECT_TRUE(window_);
		auto level = std::make_shared<frame::LevelBase>();
		static_mesh_ = frame::file::LoadStaticMeshFromFileOpenGL(
			level,
			"../Asset/Model/Torus.obj");
		EXPECT_NE(0, static_mesh_->GetMaterialId());
		EXPECT_NE(0, static_mesh_->GetPointBufferId());
		EXPECT_NE(0, static_mesh_->GetNormalBufferId());
		EXPECT_NE(0, static_mesh_->GetTextureBufferId());
		EXPECT_NE(0, static_mesh_->GetIndexBufferId());
		auto id = static_mesh_->GetIndexBufferId();
		auto index_buffer = level->GetBufferMap().at(id);
		EXPECT_LE(3456, index_buffer->GetSize());
		EXPECT_GE(6912, index_buffer->GetSize());
		EXPECT_TRUE(static_mesh_);
	}

} // End namespace test.
