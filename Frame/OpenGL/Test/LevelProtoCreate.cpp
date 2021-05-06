#include "LevelProtoCreate.h"

namespace test {

	frame::proto::Level GetLevel()
	{
		frame::proto::Level level{};
		level.set_name("test");
		level.set_default_texture_name("texture");
		return level;
	}

	frame::proto::ProgramFile GetProgramFile()
	{
		frame::proto::ProgramFile program_file{};
		frame::proto::Program program{};
		{
			program.set_name("program");
			program.set_shader("Blur");
			frame::proto::SceneType scene_type;
			scene_type.set_value(frame::proto::SceneType::QUAD);
			*program.mutable_input_scene_type() = scene_type;
			program.add_output_texture_names("texture");
		}
		*program_file.add_programs() = program;
		return program_file;
	}

	frame::proto::SceneTreeFile GetSceneFile()
	{
		frame::proto::SceneTreeFile scene_tree_file{};
		{
			frame::proto::SceneMatrix scene_matrix{};
			{
				scene_matrix.set_name("root");
			}
			*scene_tree_file.add_scene_matrices() = scene_matrix;
			frame::proto::SceneCamera scene_camera{};
			{
				scene_camera.set_name("camera");
				scene_camera.set_parent("root");
				scene_camera.set_fov_degrees(60.0f);
			}
			*scene_tree_file.add_scene_cameras() = scene_camera;
			scene_tree_file.set_default_root_name("root");
			scene_tree_file.set_default_camera_name("camera");
		}
		return scene_tree_file;
	}

	frame::proto::TextureFile GetTextureFile()
	{
		frame::proto::TextureFile texture_file{};
		frame::proto::Texture texture{};
		{
			texture.set_name("texture");
			frame::proto::Size size;
			size.set_x(-1);
			size.set_y(-1);
			*texture.mutable_size() = size;
			texture.mutable_pixel_element_size()->set_value(
				frame::proto::PixelElementSize::HALF);
			texture.mutable_pixel_structure()->set_value(
				frame::proto::PixelStructure::RGB);
		}
		*texture_file.add_textures() = texture;
		return texture_file;
	}

	frame::proto::MaterialFile GetMaterialFile()
	{
		return {};
	}

} // End namespace test.
