#pragma once

#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Scene.h"

namespace test {

	class SceneTest : public testing::Test
	{
	public:
		// OpenGL is needed as this is used by mesh.
		SceneTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
		}
		// Populate the tree with:
		//	    Matrix (contain identity)
		//	    \--- Mesh   (contain CubeUVNormal.obj)
		//	    \--- Matrix (contain displacement matrix)
		//	         \--- Mesh   (contain TorusUVNormal.obj)
		void PopulateTree()
		{
			glm::mat4 identity(1.0f);
			auto matrix_scene = std::make_shared<sgl::SceneMatrix>(identity);
			auto program = sgl::CreateProgram("Simple");
			matrix_scene->SetName("matrix_scene");
			scene_tree_->AddNode(matrix_scene);
			{
				auto cube_mesh = sgl::CreateMeshFromObjFile(
					"../Asset/Model/Cube.obj");
				auto cube = std::make_shared<sgl::SceneStaticMesh>(cube_mesh);
				cube->SetName("cube");
				cube->SetParentName("matrix_scene");
				scene_tree_->AddNode(cube);
			}
			{
				glm::mat4 disp_mat(1.0);
				disp_mat = glm::translate(disp_mat, glm::vec3(10.0, 10.0, 10.));
				auto disp =	std::make_shared<sgl::SceneMatrix>(disp_mat);
				disp->SetName("disp");
				disp->SetParentName("matrix_scene");
				scene_tree_->AddNode(disp);
				{
					auto mesh = sgl::CreateMeshFromObjFile(
						"../Asset/Model/Torus.obj");
					auto torus = std::make_shared<sgl::SceneStaticMesh>(mesh);
					torus->SetName("torus");
					torus->SetParentName("disp");
					scene_tree_->AddNode(torus);
				}
			}
		}

	protected:
		std::shared_ptr<sgl::WindowInterface> window_ = nullptr;
		std::shared_ptr<sgl::SceneInterface> scene_ = nullptr;
		std::shared_ptr<sgl::SceneTree> scene_tree_ = nullptr;
	};

} // End namespace test.