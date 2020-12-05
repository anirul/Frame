#include "DeviceTest.h"

namespace test {

	TEST_F(DeviceTest, CreateDeviceTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->GetUniqueDevice();
		EXPECT_TRUE(device_);
	}

	TEST_F(DeviceTest, StartupDeviceFailNoDefinitionTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->GetUniqueDevice();
		EXPECT_TRUE(device_);
		// No definition this should fail.
		EXPECT_THROW(device_->Startup({}, {}, {}, {}), std::exception);
	}

	TEST_F(DeviceTest, StartupDeviceWithCameraTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->GetUniqueDevice();
		device_->Startup(
			GetLevel(), 
			GetEffectFile(), 
			GetSceneFile(), 
			GetTextureFile());
		EXPECT_TRUE(device_);
	}

	frame::proto::Level DeviceTest::GetLevel() const
	{
		frame::proto::Level level{};
		level.set_level_name("test");
		level.set_default_camera_name("camera");
		level.set_default_scene_name("scene");
		level.set_default_texture_name("texture");
		return level;
	}

	frame::proto::EffectFile DeviceTest::GetEffectFile() const
	{
		frame::proto::EffectFile effect_file{};
		frame::proto::Effect effect{};
		{
			effect.set_name("effect");
			effect.set_render_type(frame::proto::Effect::SCENE);
			effect.set_shader("Blur");
			effect.add_output_textures_names("texture");
		}
		*effect_file.add_effects() = effect;
		return effect_file;
	}

	frame::proto::SceneFile DeviceTest::GetSceneFile() const
	{
		frame::proto::SceneFile scene_file{};
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
			scene_tree.set_root_node("root");
		}
		*scene_file.add_scene_trees() = scene_tree;
		return scene_file;
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

} // End namespace test.
