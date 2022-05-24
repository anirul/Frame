#pragma once

#include <gtest/gtest.h>
#include "Frame/MaterialInterface.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/Window.h"

namespace test {

	class MaterialTest : public testing::Test
	{
	public:
		MaterialTest() : window_(frame::CreateSDLOpenGL(size_))	{}

	protected:
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
		std::unique_ptr<frame::MaterialInterface> material_ = nullptr;
	};

} // End namespace test.