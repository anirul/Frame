#include "LoadTextureTest.h"
#include "Frame/Error.h"
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/OpenGL/Texture.h"

namespace test {

	TEST_F(LoadTextureTest, LoadTextureFromFloatTest)
	{
		const frame::Error& error = frame::Error::GetInstance();
		auto maybe_texture = frame::opengl::file::LoadTextureFromFloat(0.1f);
		EXPECT_TRUE(maybe_texture);
		auto texture = std::move(maybe_texture.value());
		EXPECT_TRUE(texture);
		EXPECT_EQ(1, texture->GetSize().first);
		EXPECT_EQ(1, texture->GetSize().second);
		texture->Bind();
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		error.Display(__FILE__, __LINE__ - 1);
		float f = 0.0f;
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &f);
		error.Display(__FILE__, __LINE__ - 1);
		EXPECT_FLOAT_EQ(0.1f, f);
		texture->UnBind();
	}

	TEST_F(LoadTextureTest, LoadTextureFromVec4Test)
	{
		const frame::Error& error = frame::Error::GetInstance();
		auto maybe_texture = frame::opengl::file::LoadTextureFromVec4(
				glm::vec4(0.1f, 0.2f, 0.3f, 0.4f));
		EXPECT_TRUE(maybe_texture);
		auto texture = std::move(maybe_texture.value());
		EXPECT_TRUE(texture);
		EXPECT_EQ(1, texture->GetSize().first);
		EXPECT_EQ(1, texture->GetSize().second);
		auto pair = texture->GetTexture(0);
		std::vector<float> v4;
		v4.resize(4);
		memcpy(v4.data(), pair.first, pair.second);
		EXPECT_EQ(4, v4.size());
		EXPECT_FLOAT_EQ(0.1f, v4[0]);
		EXPECT_FLOAT_EQ(0.2f, v4[1]);
		EXPECT_FLOAT_EQ(0.3f, v4[2]);
		EXPECT_FLOAT_EQ(0.4f, v4[3]);
	}

	TEST_F(LoadTextureTest, LoadTextureFromFileTest)
	{
		const frame::Error& error = frame::Error::GetInstance();
		auto maybe_texture = 
				frame::opengl::file::LoadTextureFromFile(
					frame::file::FindFile("Asset/CubeMap/PositiveX.png"),
					frame::proto::PixelElementSize_BYTE(),
					frame::proto::PixelStructure_RGB_ALPHA());
		EXPECT_TRUE(maybe_texture);
		auto texture = std::move(maybe_texture.value());
		EXPECT_TRUE(texture);
		EXPECT_EQ(1024, texture->GetSize().first);
		EXPECT_EQ(1024, texture->GetSize().second);
		const std::size_t image_size = 
			static_cast<std::size_t>(texture->GetSize().first) * 
			static_cast<std::size_t>(texture->GetSize().second) * 
			static_cast<std::size_t>(texture->GetPixelStructure());
		auto pair = texture->GetTexture(0);
		for (int position = 0; position < pair.second; position += 1025 * 2) {
			std::uint8_t* p = (std::uint8_t*)pair.first;
			EXPECT_GE(0xff, p[position]);
			EXPECT_LE(0x4e, p[position]);
		}
	}

	// TODO(anirul): Add a test for the load Cubemap from single file when
	// TODO(anirul): implemented.

	TEST_F(LoadTextureTest, LoadCubeMapFromFilesTest)
	{
		const frame::Error& error = frame::Error::GetInstance();
		auto maybe_texture =
			frame::opengl::file::LoadCubeMapTextureFromFiles(
				{ 
					frame::file::FindFile("Asset/CubeMap/PositiveX.png"),
					frame::file::FindFile("Asset/CubeMap/NegativeX.png"),
					frame::file::FindFile("Asset/CubeMap/PositiveY.png"),
					frame::file::FindFile("Asset/CubeMap/NegativeY.png"),
					frame::file::FindFile("Asset/CubeMap/PositiveZ.png"),
					frame::file::FindFile("Asset/CubeMap/NegativeZ.png")
				},
				frame::proto::PixelElementSize_BYTE(),
				frame::proto::PixelStructure_RGB_ALPHA());
		EXPECT_TRUE(maybe_texture);
		auto texture = std::move(maybe_texture.value());
		EXPECT_TRUE(texture);
		EXPECT_EQ(1024, texture->GetSize().first);
		EXPECT_EQ(1024, texture->GetSize().second);
		const std::size_t image_size = 
			static_cast<std::size_t>(texture->GetSize().first) *
			static_cast<std::size_t>(texture->GetSize().second) *
			static_cast<std::size_t>(texture->GetPixelStructure());
		for (const auto direction : {0, 1, 2, 3, 4, 5})
		{
			auto pair = texture->GetTexture(direction);
			for (int position = 0; position < image_size; position += 1025 * 5)
			{
				std::uint8_t* p = (std::uint8_t*)pair.first;
				EXPECT_GE(0xff, p[position]);
				EXPECT_LE(0x45, p[position]);
			}
		}
	}

} // End namespace test.
