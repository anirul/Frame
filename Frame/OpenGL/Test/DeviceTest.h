#pragma once

#include <gtest/gtest.h>
#include "../Frame/Window.h"
#include "../Frame/DeviceInterface.h"
#include "../FrameProto/Proto.h"
#include "../OpenGLLib/Device.h"

namespace test {

	class DeviceTest : public ::testing::Test
	{
	public:
		DeviceTest()
		{
			window_ = frame::CreateSDLOpenGL({ 320, 200 });
		}

	protected:
		frame::proto::Level GetLevel() const;
		frame::proto::ProgramFile GetProgramFile() const;
		frame::proto::SceneTreeFile GetSceneFile() const;
		frame::proto::TextureFile GetTextureFile() const;
		frame::proto::MaterialFile GetMaterialFile() const;

	protected:
		std::shared_ptr<frame::WindowInterface> window_ = nullptr;
		std::shared_ptr<frame::DeviceInterface> device_ = nullptr;
	};

} // End namespace test.