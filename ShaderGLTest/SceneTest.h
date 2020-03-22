#pragma once

#include <gtest/gtest.h>
#include "OpenGLTest.h"
#include "../ShaderGLLib/Scene.h"

namespace test {

	class SceneTest : public OpenGLTest
	{
	public:
		// OpenGL is needed as this is used by mesh.
		SceneTest() : OpenGLTest()
		{
			GLContextAndGlewInit();
		}
		// Populate the tree with:
		//	    Matrix (contain identity)
		//	    \--- Mesh   (contain CubeUVNormal.obj)
		//	    \--- Matrix (contain displacement matrix)
		//	         \--- Mesh   (contain TorusUVNormal.obj)
		void PopulateTree()
		{
			sgl::matrix identity =
			{
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1,
			};
			auto matrix_scene = std::make_shared<sgl::SceneMatrix>(identity);
			scene_tree_->AddNode(matrix_scene);
			{
				auto mesh = std::make_shared<sgl::Mesh>(
					"../asset/CubeUVNormal.obj");
				scene_tree_->AddNode(
					std::make_shared<sgl::SceneMesh>(mesh),
					matrix_scene);
			}
			{
				sgl::matrix disp =
				{
					1, 0, 0, 10,
					0, 1, 0, 10,
					0, 0, 1, 10,
					0, 0, 0, 1,
				};
				auto disp_scene =
					std::make_shared<sgl::SceneMatrix>(disp);
				scene_tree_->AddNode(disp_scene, matrix_scene);
				{
					auto mesh = std::make_shared<sgl::Mesh>(
						"../asset/TorusUVNormal.obj");
					scene_tree_->AddNode(
						std::make_shared<sgl::SceneMesh>(mesh),
						disp_scene);
				}
			}
		}

	protected:
		std::shared_ptr<sgl::Scene> scene_ = nullptr;
		std::shared_ptr<sgl::SceneTree> scene_tree_ = nullptr;
	};

} // End namespace test.