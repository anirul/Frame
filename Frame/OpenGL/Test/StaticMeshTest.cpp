#include "StaticMeshTest.h"

namespace test {

	TEST_F(StaticMeshTest, CreateCubeMeshTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(static_mesh_);
		EXPECT_TRUE(window_);
		auto program = frame::opengl::CreateProgram("SceneSimple");
		static_mesh_ = sgl::CreateStaticMeshFromObjFile(
			"../Asset/Model/Cube.obj");
		EXPECT_NE(0, static_mesh_->PointBuffer().GetId());
		EXPECT_NE(0, static_mesh_->NormalBuffer().GetId());
		EXPECT_NE(0, static_mesh_->TextureBuffer().GetId());
		EXPECT_NE(0, static_mesh_->IndexBuffer().GetId());
		EXPECT_LE(18, static_mesh_->IndexSize());
		EXPECT_GE(36, static_mesh_->IndexSize());
		EXPECT_TRUE(static_mesh_);
	}

	TEST_F(StaticMeshTest, CreateTorusMeshTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(static_mesh_);
		EXPECT_TRUE(window_);
		auto program = sgl::CreateProgram("SceneSimple");
		static_mesh_ = sgl::CreateStaticMeshFromObjFile(
			"../Asset/Model/Torus.obj");
		EXPECT_NE(0, static_mesh_->PointBuffer().GetId());
		EXPECT_NE(0, static_mesh_->NormalBuffer().GetId());
		EXPECT_NE(0, static_mesh_->TextureBuffer().GetId());
		EXPECT_NE(0, static_mesh_->IndexBuffer().GetId());
		EXPECT_LE(3456, static_mesh_->IndexSize());
		EXPECT_GE(6912, static_mesh_->IndexSize());
		EXPECT_TRUE(static_mesh_);
	}

} // End namespace test.
