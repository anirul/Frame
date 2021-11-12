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
		DeviceTest()
		{
			const auto size = 
				std::make_pair<std::uint32_t, std::uint32_t>(320, 200);
			window_ = frame::CreateSDLOpenGL(size);
			device_ = window_->GetUniqueDevice();
			auto maybe_level = frame::proto::ParseLevelOpenGL(
				size,
				frame::proto::GetLevel(),
				frame::proto::GetProgramFile(),
				frame::proto::GetSceneFile(),
				frame::proto::GetTextureFile(),
				frame::proto::GetMaterialFile());
			if (!maybe_level) throw std::runtime_error("Couldn't create level.");
			level_ = std::move(maybe_level.value());
		}

	protected:
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
		// TODO(anirul): Check why this is still a shared ptr.
		std::shared_ptr<frame::DeviceInterface> device_ = nullptr;
		std::unique_ptr<frame::LevelInterface> level_ = nullptr;
	};

} // End namespace test.