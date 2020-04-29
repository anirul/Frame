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

			scene_tree_->AddNode(matrix_scene);
			{
				auto mesh = std::make_shared<sgl::Mesh>(
					"../Asset/Model/Cube.obj",
					program);
				scene_tree_->AddNode(
					std::make_shared<sgl::SceneMesh>(mesh),
					matrix_scene);
			}
			{
				glm::mat4 disp(1.0);
				disp = glm::translate(disp, glm::vec3(10.0, 10.0, 10.));
				auto disp_scene =
					std::make_shared<sgl::SceneMatrix>(disp);
				scene_tree_->AddNode(disp_scene, matrix_scene);
				{
					auto mesh = std::make_shared<sgl::Mesh>(
						"../Asset/Model/Torus.obj", 
						program);
					scene_tree_->AddNode(
						std::make_shared<sgl::SceneMesh>(mesh),
						disp_scene);
				}
			}
		}

	protected:
		std::shared_ptr<sgl::Window> window_ = nullptr;
		std::shared_ptr<sgl::Scene> scene_ = nullptr;
		std::shared_ptr<sgl::SceneTree> scene_tree_ = nullptr;
	};

} // End namespace test.