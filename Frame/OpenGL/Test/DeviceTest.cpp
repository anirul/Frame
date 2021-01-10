#include "DeviceTest.h"

namespace test {

	TEST_F(DeviceTest, CreateDeviceTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->GetUniqueDevice();
		EXPECT_TRUE(device_);
	}

	TEST_F(DeviceTest, StartupDeviceWithCameraTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->GetUniqueDevice();
		auto level = LoadLevelFromProto(
			std::make_pair<std::uint32_t, std::uint32_t>(32, 32),
			GetLevel(),
			GetProgramFile(),
			GetSceneFile(), 
			GetTextureFile(),
			GetMaterialFile());
		device_->Startup(level);
		EXPECT_TRUE(device_);
	}

	frame::proto::Level DeviceTest::GetLevel() const
	{
		frame::proto::Level level{};
		level.set_name("test");
		level.set_default_camera_name("camera");
		level.set_default_scene_name("scene");
		level.set_default_texture_name("texture");
		return level;
	}

	frame::proto::ProgramFile DeviceTest::GetProgramFile() const
	{
		frame::proto::ProgramFile program_file{};
		frame::proto::Program program{};
		{
			program.set_name("program");
			program.set_shader("Blur");
			program.add_output_texture_names("texture");
		}
		*program_file.add_programs() = program;
		return program_file;
	}

	frame::proto::SceneTreeFile DeviceTest::GetSceneFile() const
	{
		frame::proto::SceneTreeFile scene_tree_file{};
		frame::proto::SceneTree scene_tree{};
		{
			scene_tree.set_name("scene");
			frame::proto::SceneMatrix scene_matrix{};
			{
				scene_matrix.set_name("root");
			}
			*scene_tree.add_scene_matrices() = scene_matrix;
			frame::proto::SceneCamera scene_camera{};
			{
				scene_camera.set_name("camera");
				scene_camera.set_parent("root");
			}
			*scene_tree.add_scene_cameras() = scene_camera;
			scene_tree.set_default_root_name("root");
		}
		*scene_tree_file.add_scene_trees() = scene_tree;
		return scene_tree_file;
	}

	frame::proto::TextureFile DeviceTest::GetTextureFile() const
	{
		frame::proto::TextureFile texture_file{};
		frame::proto::Texture texture{};
		{
			texture.set_name("texture");
			texture.mutable_pixel_element_size()->set_value(
				frame::proto::PixelElementSize::HALF);
			texture.mutable_pixel_structure()->set_value(
				frame::proto::PixelStructure::RGB);
		}
		*texture_file.add_textures() = texture;
		return texture_file;
	}

	frame::proto::MaterialFile DeviceTest::GetMaterialFile() const
	{
		return {};
	}

} // End namespace test.
