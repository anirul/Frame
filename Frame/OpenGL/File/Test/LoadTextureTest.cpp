#include "LoadTextureTest.h"
#include "Frame/Error.h"
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/OpenGL/Texture.h"

namespace test {

	TEST_F(LoadTextureTest, LoadTextureFromFloatTest)
	{
		const frame::Error& error = frame::Error::GetInstance();
		std::shared_ptr<frame::TextureInterface> texture = nullptr;
		ASSERT_FALSE(texture);
		texture = frame::opengl::file::LoadTextureFromFloat(0.1f);
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
		std::shared_ptr<frame::opengl::Texture> texture = nullptr;
		ASSERT_FALSE(texture);
		texture = std::dynamic_pointer_cast<frame::opengl::Texture>(
				frame::opengl::file::LoadTextureFromVec4(
					glm::vec4(0.1f, 0.2f, 0.3f, 0.4f)));
		EXPECT_TRUE(texture);
		EXPECT_EQ(1, texture->GetSize().first);
		EXPECT_EQ(1, texture->GetSize().second);
		std::vector<float> v4(4, 0.0f);
		texture->GetTexture(v4.data());
		EXPECT_FLOAT_EQ(0.1f, v4[0]);
		EXPECT_FLOAT_EQ(0.2f, v4[1]);
		EXPECT_FLOAT_EQ(0.3f, v4[2]);
		EXPECT_FLOAT_EQ(0.4f, v4[3]);
	}

	TEST_F(LoadTextureTest, LoadTextureFromFileTest)
	{
		const frame::Error& error = frame::Error::GetInstance();
		std::shared_ptr<frame::opengl::Texture> texture = nullptr;
		ASSERT_FALSE(texture);
		texture = std::dynamic_pointer_cast<frame::opengl::Texture>(
				frame::opengl::file::LoadTextureFromFile(
					frame::file::FindFile("Asset/CubeMap/PositiveX.png"),
					frame::proto::PixelElementSize_BYTE(),
					frame::proto::PixelStructure_RGB()));
		EXPECT_TRUE(texture);
		EXPECT_EQ(1024, texture->GetSize().first);
		EXPECT_EQ(1024, texture->GetSize().second);
		const std::size_t image_size = 
			static_cast<std::size_t>(texture->GetSize().first) * 
			static_cast<std::size_t>(texture->GetSize().second) * 
			static_cast<std::size_t>(texture->GetPixelStructure().value());
		std::vector<std::uint8_t> vec(image_size, 0);
		texture->GetTexture(vec.data());
		int position = 0;
		for (int position = 0; position < image_size; position += 1025 * 2) {
			EXPECT_GE(0xff, vec[position]);
			EXPECT_LE(0x4e, vec[position]);
		}
	}

	// TODO(anirul): Add a test for the load Cubemap from single file when
	// TODO(anirul): implemented.

	TEST_F(LoadTextureTest, LoadCubeMapFromFilesTest)
	{
		const frame::Error& error = frame::Error::GetInstance();
		std::shared_ptr<frame::opengl::TextureCubeMap> texture = nullptr;
		ASSERT_FALSE(texture);
		texture = std::dynamic_pointer_cast<frame::opengl::TextureCubeMap>(
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
				frame::proto::PixelStructure_RGB()));
		EXPECT_TRUE(texture);
		EXPECT_EQ(1024, texture->GetSize().first);
		EXPECT_EQ(1024, texture->GetSize().second);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		error.Display(__FILE__, __LINE__ - 1);
		const std::size_t image_size = 
			static_cast<std::size_t>(texture->GetSize().first) *
			static_cast<std::size_t>(texture->GetSize().second) *
			static_cast<std::size_t>(texture->GetPixelStructure().value());
		std::array<std::vector<std::uint8_t>, 6> arr_vec = { 
			std::vector<std::uint8_t>(image_size, 0),
			std::vector<std::uint8_t>(image_size, 0),
			std::vector<std::uint8_t>(image_size, 0),
			std::vector<std::uint8_t>(image_size, 0),
			std::vector<std::uint8_t>(image_size, 0),
			std::vector<std::uint8_t>(image_size, 0)
		};
		std::array<void*, 6> arr_ptr = { 
			arr_vec[0].data(),
			arr_vec[1].data(),
			arr_vec[2].data(),
			arr_vec[3].data(),
			arr_vec[4].data(),
			arr_vec[5].data()
		};
		texture->GetTextureCubeMap(arr_ptr);
		for (const auto vec : arr_vec)
		{
			int position = 0;
			for (int position = 0; position < image_size; position += 1025 * 5)
			{
				EXPECT_GE(0xff, vec[position]);
				EXPECT_LE(0x45, vec[position]);
			}
		}
	}

} // End namespace test.
