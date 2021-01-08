#pragma once

#include <gtest/gtest.h>
#include "Frame/Error.h"
#include "Frame/MaterialInterface.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/Window.h"

namespace test {

	class MaterialTest : public testing::Test
	{
	public:
		MaterialTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
			error_.SetWindowPtr(nullptr);
		}

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		frame::Error& error_ = frame::Error::GetInstance();
		std::shared_ptr<frame::MaterialInterface> material_ = nullptr;
	};

} // End namespace test.