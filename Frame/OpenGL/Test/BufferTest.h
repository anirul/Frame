#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/BufferInterface.h"

namespace test {

	class BufferTest : public testing::Test
	{
	public:
		BufferTest()
		{
			window_ = frame::CreateSDLOpenGL({320, 200});
		}

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		std::shared_ptr<frame::BufferInterface> buffer_ = nullptr;
	};

} // End namespace test.
