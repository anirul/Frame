#pragma once

#include <gtest/gtest.h>
#include "Frame/DeviceInterface.h"
#include "Frame/Level.h"
#include "Frame/Window.h"
#include "Frame/OpenGL/Renderer.h"
#include "Frame/Proto/ParseLevel.h"
#include "Frame/File/FileSystem.h"

namespace test {

	class RendererTest : public testing::Test
	{
	public:
		RendererTest() : window_(frame::CreateSDLOpenGL(size_)) {}

	public:
		bool LoadDefaultLevel()
		{
			auto maybe_level = frame::proto::ParseLevelOpenGL(
				size_,
				frame::proto::LoadProtoFromJsonFile<frame::proto::Level>(
					frame::file::FindFile("Asset/Json/RendererTest.json")));
			if (maybe_level)
			{
				level_ = std::move(maybe_level.value());
				return true;
			}
			return false;
		}

	protected:
		std::pair<std::uint32_t, std::uint32_t> size_ = { 320, 200 };
		std::unique_ptr<frame::WindowInterface> window_ = nullptr;
		std::unique_ptr<frame::LevelInterface> level_ = nullptr;
		std::unique_ptr<frame::opengl::Renderer> renderer_ = nullptr;
	};

} // End namespace test.
