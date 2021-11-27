#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/StaticMeshInterface.h"

namespace test {

	class StaticMeshTest : public testing::Test
	{
	public:
		StaticMeshTest() : window_(frame::CreateSDLOpenGL(size_)) {}

	protected:
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
		std::unique_ptr<frame::StaticMeshInterface> static_mesh_ = nullptr;
	};

} // End namespace test.
