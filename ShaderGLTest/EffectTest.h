#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Effect.h"
#include "../ShaderGLLib/Window.h"

namespace test {

	class EffectTest : public ::testing::Test
	{
	public:
		EffectTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<sgl::WindowInterface> window_ = nullptr;
		sgl::Error& error_ = sgl::Error::GetInstance();
	};

} // End namespace test.
