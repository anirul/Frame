#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/File/Obj.h"

namespace test {

	class LoadStaticMeshTest : public testing::Test
	{
	public:
		LoadStaticMeshTest() 
		{ 
			window_ = frame::CreateSDLOpenGL({ 320, 200 }); 
		}

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.
