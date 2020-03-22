#pragma once

#include <gtest/gtest.h>
#include "OpenGLTest.h"
#include "../ShaderGLLib/Device.h"

namespace test {

	class DeviceTest : public OpenGLTest
	{
	public:
		DeviceTest() : OpenGLTest() {}

	protected:
		std::shared_ptr<sgl::Device> device_ = nullptr;
	};

} // End namespace test.