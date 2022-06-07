#pragma once

#include <gtest/gtest.h>
#include "Frame/WindowInterface.h"

namespace test {

	class WindowTest : public testing::Test
	{
	public:
		WindowTest() = default;

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
	};

} // End namespace test.