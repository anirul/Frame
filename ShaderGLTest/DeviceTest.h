#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Device.h"

namespace test {

	class DeviceTest : public ::testing::Test
	{
	public:
		DeviceTest()
		{
			window_ = sgl::CreateSDLOpenGL({ 320, 200 });
		}

	protected:
		frame::proto::Level GetLevel() const;
		frame::proto::EffectFile GetEffectFile() const;
		frame::proto::SceneFile GetSceneFile() const;
		frame::proto::TextureFile GetTextureFile() const;

	protected:
		std::shared_ptr<sgl::WindowInterface> window_ = nullptr;
		std::shared_ptr<sgl::Device> device_ = nullptr;
	};

} // End namespace test.