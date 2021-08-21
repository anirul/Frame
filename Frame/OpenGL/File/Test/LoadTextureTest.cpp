#include "LoadTextureTest.h"
#include "Frame/Error.h"
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/OpenGL/Texture.h"

namespace test {

	TEST_F(LoadTextureTest, LoadTextureFromFloatTest)
	{
		const frame::Error& error = frame::Error::GetInstance();
		std::unique_ptr<frame::TextureInterface> texture = 
			frame::opengl::file::LoadTextureFromFloat(0.1f);
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
		std::unique_ptr<frame::TextureInterface> texture = 
			frame::opengl::file::LoadTextureFromVec4(
				glm::vec4(0.1f, 0.2f, 0.3f, 0.4f));
		EXPECT_TRUE(texture);
		EXPECT_EQ(1, texture->GetSize().first);
		EXPECT_EQ(1, texture->GetSize().second);
		std::vector<std::any> v4 = texture->GetTexture();
		EXPECT_EQ(4, v4.size());
		EXPECT_FLOAT_EQ(0.1f, std::any_cast<float>(v4[0]));
		EXPECT_FLOAT_EQ(0.2f, std::any_cast<float>(v4[1]));
		EXPECT_FLOAT_EQ(0.3f, std::any_cast<float>(v4[2]));
		EXPECT_FLOAT_EQ(0.4f, std::any_cast<float>(v4[3]));
	}

	TEST_F(LoadTextureTest, LoadTextureFromFileTest)
	{
		const frame::Error& error = frame::Error::GetInstance();
		std::shared_ptr<frame::TextureInterface> texture = 
				frame::opengl::file::LoadTextureFromFile(
					frame::file::FindFile("Asset/CubeMap/PositiveX.png"),
					frame::proto::PixelElementSize_BYTE(),
					frame::proto::PixelStructure_RGB());
		EXPECT_TRUE(texture);
		EXPECT_EQ(1024, texture->GetSize().first);
		EXPECT_EQ(1024, texture->GetSize().second);
		const std::size_t image_size = 
			static_cast<std::size_t>(texture->GetSize().first) * 
			static_cast<std::size_t>(texture->GetSize().second) * 
			static_cast<std::size_t>(texture->GetPixelStructure());
		std::vector<std::any> vec = texture->GetTexture();
		for (int position = 0; position < image_size; position += 1025 * 2) {
			EXPECT_GE(0xff, std::any_cast<std::uint8_t>(vec[position]));
			EXPECT_LE(0x4e, std::any_cast<std::uint8_t>(vec[position]));
		}
	}

	// TODO(anirul): Add a test for the load Cubemap from single file when
	// TODO(anirul): implemented.

	TEST_F(LoadTextureTest, LoadCubeMapFromFilesTest)
	{
		const frame::Error& error = frame::Error::GetInstance();
		std::unique_ptr<frame::TextureInterface> texture =
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
				frame::proto::PixelStructure_RGB());
		EXPECT_TRUE(texture);
		EXPECT_EQ(1024, texture->GetSize().first);
		EXPECT_EQ(1024, texture->GetSize().second);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		error.Display(__FILE__, __LINE__ - 1);
		const std::size_t image_size = 
			static_cast<std::size_t>(texture->GetSize().first) *
			static_cast<std::size_t>(texture->GetSize().second) *
			static_cast<std::size_t>(texture->GetPixelStructure());
		for (const auto direction : {0, 1, 2, 3, 4, 5})
		{
			std::vector<std::any> any_vec;
			any_vec = texture->GetTexture(direction);
			for (int position = 0; position < image_size; position += 1025 * 5)
			{
				EXPECT_GE(0xff, std::any_cast<std::uint8_t>(any_vec[position]));
				EXPECT_LE(0x45, std::any_cast<std::uint8_t>(any_vec[position]));
			}
		}
	}

} // End namespace test.
