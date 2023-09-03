#pragma once

#include <gtest/gtest.h>

#include "frame/buffer_interface.h"
#include "frame/window_factory.h"

namespace test {

	class BufferTest : public testing::Test {
	public:
		BufferTest()
			: window_(frame::CreateNewWindow(frame::DrawingTargetEnum::NONE)) {}

	protected:
		const glm::uvec2 size_ = { 330, 200 };
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
		std::unique_ptr<frame::BufferInterface> buffer_ = nullptr;
	};

}  // End namespace test.
