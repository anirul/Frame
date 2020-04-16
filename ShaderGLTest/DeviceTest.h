#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Device.h"

namespace test {

	class DeviceTest : public testing::Test
	{
	public:
		DeviceTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
		}

	protected:
		std::shared_ptr<sgl::Window> window_ = nullptr;
		std::shared_ptr<sgl::Device> device_ = nullptr;
	};

} // End namespace test.