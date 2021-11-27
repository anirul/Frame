#pragma once

#include <gtest/gtest.h>
#include "Frame/Window.h"
#include "Frame/DeviceInterface.h"
#include "Frame/Proto/ParseLevel.h"
#include "Frame/Proto/Proto.h"
#include "Frame/Proto/ProtoLevelCreate.h"
#include "Frame/OpenGL/Device.h"

namespace test {

	class DeviceTest : public ::testing::Test
	{
	public:
		DeviceTest() : window_(frame::CreateSDLOpenGL(size_))
		{
			auto maybe_level = frame::proto::ParseLevelOpenGL(
				size_,
				frame::proto::GetLevel(),
				frame::proto::GetProgramFile(),
				frame::proto::GetSceneFile(),
				frame::proto::GetTextureFile(),
				frame::proto::GetMaterialFile());
			if (!maybe_level) 
				throw std::runtime_error("Couldn't create level.");
			level_ = std::move(maybe_level.value());
		}

	protected:
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
		std::unique_ptr<frame::LevelInterface> level_ = nullptr;
	};

} // End namespace test.