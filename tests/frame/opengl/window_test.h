#pragma once

#include <gtest/gtest.h>
#include "frame/window_interface.h"

namespace test {

	class WindowTest : public testing::Test
	{
	public:
		WindowTest() = default;

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.
