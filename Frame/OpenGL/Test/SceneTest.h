#pragma once

#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Frame/File/LoadStaticMesh.h"
#include "Frame/LevelInterface.h"
#include "Frame/OpenGL/Program.h"
#include "Frame/SceneMatrix.h"
#include "Frame/SceneTree.h"
#include "Frame/SceneStaticMesh.h"
#include "Frame/Window.h"

namespace test {

	class SceneTest : public testing::Test
	{
	public:
		// OpenGL is needed as this is used by mesh.
		SceneTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
		}
		// Populate the tree with:
		//	    Matrix (contain identity)
		//	    \--- Mesh   (contain CubeUVNormal.obj)
		//	    \--- Matrix (contain displacement matrix)
		//	         \--- Mesh   (contain TorusUVNormal.obj)
		void PopulateTree(const std::shared_ptr<frame::LevelInterface> level)
		{
			glm::mat4 identity(1.0f);
			auto matrix_scene = std::make_shared<frame::SceneMatrix>(identity);
			auto program = frame::opengl::CreateProgram("SceneSimple");
			matrix_scene->SetName("matrix_scene");
			scene_tree_->AddNode(matrix_scene);
			{
				auto cube_mesh = frame::file::LoadStaticMeshFromFileOpenGL(
					level,
					"../Asset/Model/Cube.obj");
				auto cube = std::make_shared<frame::SceneStaticMesh>(cube_mesh);
				cube->SetName("cube");
				cube->SetParentName("matrix_scene");
				scene_tree_->AddNode(cube);
			}
			{
				glm::mat4 disp_mat(1.0);
				disp_mat = glm::translate(disp_mat, glm::vec3(10.0, 10.0, 10.));
				auto disp =	std::make_shared<frame::SceneMatrix>(disp_mat);
				disp->SetName("disp");
				disp->SetParentName("matrix_scene");
				scene_tree_->AddNode(disp);
				{
					auto mesh = frame::file::LoadStaticMeshFromFileOpenGL(
						level,
						"../Asset/Model/Torus.obj");
					auto torus = std::make_shared<frame::SceneStaticMesh>(mesh);
					torus->SetName("torus");
					torus->SetParentName("disp");
					scene_tree_->AddNode(torus);
				}
			}
		}

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		std::shared_ptr<frame::SceneNodeInterface> scene_ = nullptr;
		std::shared_ptr<frame::SceneTree> scene_tree_ = nullptr;
	};

} // End namespace test.